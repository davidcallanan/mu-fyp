#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <variant>
#include <string>
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "evaluate_structval.hpp"
#include "evaluate_hardval.hpp"
#include "t_hardval.hpp"
#include "t_types.hpp"
#include "t_smooth_value.hpp"
#include "merge_smooth_value.hpp"
#include "create_value_symbol_table.hpp"
#include "process_map_body.hpp"
#include "get_underlying_type.hpp"
#include "is_subset_type.hpp"

static bool determine_has_leaf(const Type& type) {
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&type)) {
		return (*p_v_map)->leaf_type != nullptr || (*p_v_map)->leaf_hardval != nullptr;
	}
	
	// what was I thinking here.
	// if (auto p_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
	// 	return true;
	// }
	
	return false;
}

static SmoothValue access_variable(
	IrGenCtx& igc,
	const Type& node
) {
	std::string target_name;
	std::shared_ptr<Type>* underlying_type = nullptr;
	
	if (auto p = std::get_if<std::shared_ptr<TypeVarAccess>>(&node)) {
		target_name = (*p)->target_name;
		underlying_type = &(*p)->underlying_type;
	} else if (auto p = std::get_if<std::shared_ptr<TypeVarWalrus>>(&node)) {
		target_name = (*p)->name;
		underlying_type = &(*p)->underlying_type;
	} else if (auto p = std::get_if<std::shared_ptr<TypeVarAssign>>(&node)) {
		target_name = (*p)->name;
		underlying_type = &(*p)->underlying_type;
	} else {
		fprintf(stderr, "only TypeVarWalrus, TypeVarAssign, and TypeVarAccess can access a variable!\n");
		exit(1);
	}
	
	std::string var_name = "m_" + target_name;
	std::optional<ValueSymbolTableEntry> o_entry = igc.value_table->get(var_name);
	
	if (!o_entry.has_value()) {
		std::string sym_var_name = "ms_:" + target_name;
		o_entry = igc.value_table->get(sym_var_name);
		
		if (!o_entry.has_value()) {
			fprintf(stderr, "This variable %s was not actually present in our value table\n", target_name.c_str());
			exit(1);
		}
	}
	
	const ValueSymbolTableEntry& entry = o_entry.value();
	
	*underlying_type = std::make_shared<Type>(entry.type);
	
	llvm::Value* loaded = igc.builder.CreateLoad(
		entry.ir_type,
		entry.alloca_ptr
	);
	
	return SmoothValue{
		loaded,
		entry.type,
		entry.has_leaf,
	};
}

static SmoothValue access_member(
	IrGenCtx& igc,
	const SmoothValue& target_smooth,
	const std::string& sym
) {
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&target_smooth.type)) {
		const auto& v_map = *p_v_map;
		
		std::string sym_key = ":" + sym;
		
		if (v_map->sym_inputs.find(sym_key) == v_map->sym_inputs.end()) {
			fprintf(stderr, "Symbol %s not really available here", sym.c_str());
			exit(1);
		}
		
		const Type& unclear_type = *v_map->sym_inputs.at(sym_key);
		Type sym_type = get_underlying_type(unclear_type);
		
		// i know this logic is terrible but performance is not a concern for me.
		
		size_t field_index = (target_smooth.has_leaf ? 1 : 0);
		
		for (const auto& [sym_name, _] : v_map->sym_inputs) {
			if (sym_name == sym_key) {
				break;
			}
			
			field_index++;
		}
		
		llvm::Value* extracted = igc.builder.CreateExtractValue(target_smooth.struct_value, field_index);
		
		// pointers are typically disallowed on their own, and must be wrapped into a leaf map.
		// exception is made for syms of maps to prevent infinite recursion.
		// this is why we must deal with raw pointer here and wrap it back up.
		// we gradually wrap up pointers as they are used, lazily.
		
		if (auto p_pointer = std::get_if<std::shared_ptr<TypePointer>>(&sym_type)) {
			fprintf(stderr, "is it even getting here. %s\n", sym.c_str());
			
			llvm::StructType* wrapped_pointer = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ extracted->getType() });
			llvm::Value* final_pointer = llvm::UndefValue::get(wrapped_pointer);
			final_pointer = igc.builder.CreateInsertValue(final_pointer, extracted, 0);
			
			return SmoothValue{
				final_pointer,
				sym_type,
				true,
			};
		}
		
		return SmoothValue{
			extracted,
			sym_type,
			determine_has_leaf(sym_type),
		};
	} else {
		fprintf(stderr, "Impossible to call a non-map with a symbol, what are you doing?\n");       
		exit(1);
	}
}

SmoothValue evaluate_structval(
	IrGenCtx& igc,
	const Type& type
) {
	if (auto p_v_var_access = std::get_if<std::shared_ptr<TypeVarAccess>>(&type)) {
		return access_variable(igc, type);
	}
	
	if (auto p_v_merged = std::get_if<std::shared_ptr<TypeMerged>>(&type)) {
		const auto& merged = **p_v_merged;
		
		if (merged.types.empty()) {
			fprintf(stderr, "merging nothing?\n");
			exit(1);
		}
		
		SmoothValue result = evaluate_structval(igc, merged.types[0]);
		
		for (size_t i = 1; i < merged.types.size(); i++) {
			SmoothValue next = evaluate_structval(igc, merged.types[i]);
			result = merge_smooth_value(igc, result, next);
		}
		
		return result;
	}
	
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&type)) {
		const TypeMap& map = **p_v_map;
		std::shared_ptr<ValueSymbolTable> map_value_table = std::make_shared<ValueSymbolTable>(
			create_value_symbol_table(igc.value_table.get())
		);
		
		IrGenCtx map_igc = igc;
		map_igc.value_table = map_value_table;
		
		process_map_body(map_igc, map);
		
		std::vector<llvm::Type*> member_types;
		std::vector<llvm::Value*> member_values;
		
		if (map.leaf_hardval != nullptr) {
			const Hardval& hardval = *map.leaf_hardval;
			
			std::string type_str = "";
			
			if (map.leaf_type != nullptr) {
				auto p_rotten = std::get_if<std::shared_ptr<TypeRotten>>(map.leaf_type.get());
				
				if (p_rotten) {
					type_str = (*p_rotten)->type_str;
				}
			}
			
			llvm::Value* leaf_value = evaluate_hardval(map_igc, hardval, type_str);
			member_types.push_back(leaf_value->getType());
			member_values.push_back(leaf_value);
		}
		
		for (const auto& [sym_name, sym_type] : map.sym_inputs) {
			std::string map_sym_var_name = "ms_" + sym_name;
			std::optional<ValueSymbolTableEntry> o_entry = map_igc.value_table->get(map_sym_var_name);
			
			if (!o_entry.has_value()) {
				fprintf(stderr, "Symbol as a variable %s was not really present in the value table\n", sym_name.c_str());
				exit(1);
			}
			
			ValueSymbolTableEntry entry = o_entry.value();
			
			llvm::Value* loaded = map_igc.builder.CreateLoad(
				entry.ir_type,
				entry.alloca_ptr
			);
			
			member_types.push_back(loaded->getType());
			member_values.push_back(loaded);
		}
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, member_types);
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		for (size_t i = 0; i < member_values.size(); i++) {
			struct_value = igc.builder.CreateInsertValue(struct_value, member_values[i], i);
		}
		
		return SmoothValue{
			struct_value,
			type,
			determine_has_leaf(type),
		};
	}
	
	if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
		const TypePointer& pointer = **p_v_pointer;
		
		if (pointer.hardval == nullptr) {
			fprintf(stderr, "Evaluation cannot consist of a pointer that has no definitive value, for now (in future, would make sense to deal with pointer of variable, etc.)\n");
			exit(1);
		}
		
		llvm::Value* ptr = evaluate_hardval(igc, *pointer.hardval);
		
		std::vector<llvm::Type*> member_types;
		member_types.push_back(ptr->getType());
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, member_types);
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		struct_value = igc.builder.CreateInsertValue(struct_value, ptr, 0);
		
		return SmoothValue{
			struct_value,
			type,
			true,
		};
	}
	
	if (auto p_v_log = std::get_if<std::shared_ptr<TypeLog>>(&type)) {
		const auto& v_log = *p_v_log;
		
		if (v_log->message == nullptr) {
			llvm::Value* log_str = igc.builder.CreateGlobalStringPtr("");
			igc.builder.CreateCall(igc.puts_func, { log_str });
		} else {
			SmoothValue smooth = evaluate_structval(igc, *v_log->message);
			
			if (!smooth.has_leaf) {
				fprintf(stderr, "Not good circumstances - no leaf.\n");
				fprintf(stderr, "Actually what we got is: ");
				smooth.struct_value->print(llvm::errs());
				exit(1);
			}
			
			llvm::Value* leaf = smooth.extract_leaf(igc.builder);
			igc.builder.CreateCall(igc.puts_func, { leaf });
		}
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, {});
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		return SmoothValue{
			struct_value,
			type,
			false,
		};
	}
	
	if (auto p_v_log_d = std::get_if<std::shared_ptr<TypeLogD>>(&type)) {
		const auto& v_log_d = *p_v_log_d;

		SmoothValue smooth = evaluate_structval(igc, *v_log_d->message);

		if (!smooth.has_leaf) {
			fprintf(stderr, "No leaf so impossible to log its data (consider using \"log(...)\" for strings).\n");
			exit(1);
		}

		llvm::Value* leaf = smooth.extract_leaf(igc.builder);
		llvm::Type* leaf_type = leaf->getType();

		uint64_t bit_width = leaf_type->getPrimitiveSizeInBits();

		if (bit_width == 0) {
			fprintf(stderr, "cannot print this thing because its size is undeterminable\n");
			exit(1);
		}

		if (!leaf_type->isIntegerTy()) { // interpret the value as raw bits
			leaf = igc.builder.CreateBitCast(leaf, llvm::IntegerType::get(igc.context, bit_width));
		}

		uint64_t num_chunks = (bit_width + 63) / 64;
		uint64_t total_bits = num_chunks * 64;

		llvm::Type* i8  = llvm::Type::getInt8Ty(igc.context);
		llvm::Type* i64 = llvm::Type::getInt64Ty(igc.context);

		llvm::Value* wide = (total_bits > bit_width) // zero-extend
			? igc.builder.CreateZExt(leaf, llvm::IntegerType::get(igc.context, total_bits))
			: leaf;

		for (uint64_t i = 0; i < num_chunks; i++) {
			uint64_t shift = (num_chunks - 1 - i) * 64;
			llvm::Value* shifted = igc.builder.CreateLShr(wide, llvm::ConstantInt::get(wide->getType(), shift));
			llvm::Value* chunk = igc.builder.CreateTrunc(shifted, i64);

			llvm::Value* bl;
			llvm::Value* br;
			
			if (num_chunks == 1) {
				bl = llvm::ConstantInt::get(i8, '[');
				br = llvm::ConstantInt::get(i8, ']');
			} else if (i == 0) {
				bl = llvm::ConstantInt::get(i8, '[');
				br = llvm::ConstantInt::get(i8, '-');
			} else if (i == num_chunks - 1) {
				bl = llvm::ConstantInt::get(i8, '-');
				br = llvm::ConstantInt::get(i8, ']');
			} else {
				bl = llvm::ConstantInt::get(i8, '-');
				br = llvm::ConstantInt::get(i8, '-');
			}

			igc.builder.CreateCall(igc.log_data_func, { chunk, bl, br });
		}

		llvm::StructType* struct_type = llvm::StructType::get(igc.context, {});
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		return SmoothValue{
			struct_value,
			type,
			false,
		};
	}
	
	if (auto p_v_var_walrus = std::get_if<std::shared_ptr<TypeVarWalrus>>(&type)) {
		const auto& v_var_walrus = *p_v_var_walrus;
		
		std::string map_var_name = "m_" + v_var_walrus->name;
		std::string scoped_alloca_name = igc.value_table->scope_id() + "~" + map_var_name;
		
		SmoothValue smooth = evaluate_structval(igc, *v_var_walrus->typeval);
		llvm::Value* alloca = igc.builder.CreateAlloca(smooth.struct_value->getType(), nullptr, scoped_alloca_name);
		igc.builder.CreateStore(smooth.struct_value, alloca);
		
		
		ValueSymbolTableEntry entry{
			alloca,
			smooth.struct_value->getType(),
			smooth.type,
			smooth.has_leaf,
			v_var_walrus->is_mut,
		};
		
		igc.value_table->set(map_var_name, entry);
		
		return access_variable(igc, type);
	}
	
	if (auto p_v_var_assign = std::get_if<std::shared_ptr<TypeVarAssign>>(&type)) {
		const auto& v_var_assign = *p_v_var_assign;
		
		std::string map_var_name = "m_" + v_var_assign->name;
		std::optional<ValueSymbolTableEntry> o_entry = igc.value_table->get(map_var_name);
		
		if (!o_entry.has_value()) {
			fprintf(stderr, "So %s does not exist.\n", v_var_assign->name.c_str());
			exit(1);
		}
		
		const ValueSymbolTableEntry& existing = o_entry.value();
		
		if (!existing.is_mut) {
			fprintf(stderr, "Cannot mutate immutable variable %s, consider adding \"mut\" keyword like `mut x := 5;`\n", v_var_assign->name.c_str());
			exit(1);
		}
		
		SmoothValue smooth = evaluate_structval(igc, *v_var_assign->typeval);
		
		if (!is_subset_type(smooth.type, existing.type)) {
			fprintf(stderr, "Value is not assignable to variable \"%s\" due to incompatibility (types are not subsets)\n", v_var_assign->name.c_str());
			exit(1);
		}
		
		igc.builder.CreateStore(smooth.struct_value, existing.alloca_ptr);
		
		// as we are using stack allocations, we can mutate via stack address, bypassing immutable registers and phi logic.
		// llvm will optimize this to single-static-assignment (SSA) form for us, when possible.
		
		return access_variable(igc, type);
	}
	
	if (auto p_v_call_with_sym = std::get_if<std::shared_ptr<TypeCallWithSym>>(&type)) {
		const auto& v_call_with_sym = *p_v_call_with_sym;
		SmoothValue target_smooth = evaluate_structval(igc, *v_call_with_sym->target);
		return access_member(igc, target_smooth, v_call_with_sym->sym);
	}
	
	if (auto p_expr_multi = std::get_if<std::shared_ptr<TypeExprMulti>>(&type)) {
		const auto& v_expr_multi = **p_expr_multi;
		llvm::Value* result = nullptr;
		bool is_float = false; // we will not allow combining int and float operations implicitely.

		for (const auto& op_numeric : v_expr_multi.ops) {
			SmoothValue smooth = evaluate_structval(igc, *op_numeric.operand);
			llvm::Value* val = smooth.extract_leaf(igc.builder);

			if (result == nullptr) {
				is_float = val->getType()->isFloatingPointTy();
				result = is_float ? llvm::ConstantFP::get(val->getType(), 1.0) : llvm::ConstantInt::get(val->getType(), 1);
			} else if (val->getType()->isFloatingPointTy() != is_float) {
				fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
				exit(1);
			}

			if (!is_float) { // i don't care about floats for now.
				uint32_t result_bits = result->getType()->getIntegerBitWidth();
				uint32_t val_bits = val->getType()->getIntegerBitWidth();
				
				if (result_bits < val_bits) {
					result = igc.builder.CreateZExt(result, val->getType());
				} else if (val_bits < result_bits) {
					val = igc.builder.CreateZExt(val, result->getType());
				}
			}

			if (op_numeric.op == "*") {
				result = is_float ? igc.builder.CreateFMul(result, val) : igc.builder.CreateMul(result, val);
			} else if (op_numeric.op == "/") {
				result = is_float ? igc.builder.CreateFDiv(result, val) : igc.builder.CreateSDiv(result, val);
				// todo: is signed division appropriate in all cases? probably need to adjust this.
			} else {
				fprintf(stderr, "Some bizarre operator was encountered %s (multiplicative)\n", op_numeric.op.c_str());
				exit(1);
			}
		}

		if (result == nullptr) {
			fprintf(stderr, "multiplicative expression had no operations.\n");
			exit(1);
		}

		llvm::StructType* struct_type = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ result->getType() });
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		struct_value = igc.builder.CreateInsertValue(struct_value, result, 0);
		
		return SmoothValue{ struct_value, type, true };
	}

	if (auto p_expr_addit = std::get_if<std::shared_ptr<TypeExprAddit>>(&type)) {
		const auto& v_expr_addit = **p_expr_addit;
		llvm::Value* result = nullptr;
		bool is_float = false;

		for (const auto& op_numeric : v_expr_addit.ops) {
			SmoothValue smooth = evaluate_structval(igc, *op_numeric.operand);
			llvm::Value* val = smooth.extract_leaf(igc.builder);

			if (result == nullptr) {
				is_float = val->getType()->isFloatingPointTy();
				result = is_float ? llvm::ConstantFP::get(val->getType(), 0.0) : llvm::ConstantInt::get(val->getType(), 0);
			} else if (val->getType()->isFloatingPointTy() != is_float) {
				fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
				exit(1);
			}

			if (!is_float) { // dont care about floats for now.
				uint32_t result_bits = result->getType()->getIntegerBitWidth();
				uint32_t val_bits = val->getType()->getIntegerBitWidth();
				
				if (result_bits < val_bits) {
					result = igc.builder.CreateZExt(result, val->getType());
				} else if (val_bits < result_bits) {
					val = igc.builder.CreateZExt(val, result->getType());
				}
			}

			if (op_numeric.op == "+") {
				result = is_float ? igc.builder.CreateFAdd(result, val) : igc.builder.CreateAdd(result, val);
			} else if (op_numeric.op == "-") {
				result = is_float ? igc.builder.CreateFSub(result, val) : igc.builder.CreateSub(result, val);
			} else {
				fprintf(stderr, "Some bizarre operator was encountered %s (additive)\n", op_numeric.op.c_str());
				exit(1);
			}
		}

		if (result == nullptr) {
			fprintf(stderr, "additive expression had no operations.\n");
			exit(1);
		}

		llvm::StructType* struct_type = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ result->getType() });
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		struct_value = igc.builder.CreateInsertValue(struct_value, result, 0);
		
		return SmoothValue{ struct_value, type, true };
	}

	fprintf(stderr, "Unhandled scenario when handling evaluation\n");
	exit(1);
}
