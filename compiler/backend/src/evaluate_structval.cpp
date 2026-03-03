#include <algorithm>
#include <bit>
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
#include "t_smooth.hpp"
#include "t_smooth_fwd.hpp"
#include "llvm_value.hpp"
#include "extract_leaf.hpp"
#include "merge_smooth.hpp"
#include "create_value_symbol_table.hpp"
#include "process_map_body.hpp"
#include "get_underlying_type.hpp"
#include "is_subset_type.hpp"
#include "llvm_to_smooth_bool.hpp"
#include "structwrap.hpp"
#include "is_structwrappable.hpp"
#include "smooth_type.hpp"
#include "llvm_to_smooth.hpp"

static bool determine_has_leaf(const Type& type) {
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&type)) {
		return (*p_v_map)->leaf_type.has_value() || (*p_v_map)->leaf_hardval.has_value();
	}
	
	// what was I thinking here.
	// if (auto p_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
	// 	return true;
	// }
	
	return false;
}

static Smooth access_variable(
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

	return llvm_to_smooth(entry.type, loaded);
}

static Smooth access_member(
	IrGenCtx& igc,
	std::shared_ptr<SmoothStructval> target_smooth,
	const std::string& sym
) {
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&target_smooth->type)) {
		const auto& v_map = *p_v_map;
		
		std::string sym_key = ":" + sym;
		
		if (v_map->sym_inputs.find(sym_key) == v_map->sym_inputs.end()) {
			fprintf(stderr, "Symbol %s not really available here", sym.c_str());
			exit(1);
		}
		
		const Type& unclear_type = *v_map->sym_inputs.at(sym_key);
		Type sym_type = get_underlying_type(unclear_type);
		
		// i know this logic is terrible but performance is not a concern for me.
		
		size_t field_index = (target_smooth->has_leaf ? 1 : 0);
		
		for (const auto& [sym_name, _] : v_map->sym_inputs) {
			if (sym_name == sym_key) {
				break;
			}
			
			field_index++;
		}
		
		llvm::Value* extracted = igc.builder.CreateExtractValue(target_smooth->value, field_index);

		// pointers are typically disallowed on their own, and must be wrapped into a leaf map.
		// exception is made for syms of maps to prevent infinite recursion.
		// this is why we must deal with raw pointer here and wrap it back up.
		// we gradually wrap up pointers as they are used, lazily.
		
		if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&sym_type)) {
			fprintf(stderr, "is it even getting here. %s\n", sym.c_str());
			
			llvm::StructType* wrapped_pointer = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ extracted->getType() });
			llvm::Value* final_pointer = llvm::UndefValue::get(wrapped_pointer);
			final_pointer = igc.builder.CreateInsertValue(final_pointer, extracted, 0);

			auto actual_map = std::make_shared<TypeMap>(TypeMap{
				unclear_type,
				std::nullopt,
				nullptr,
				nullptr,
				{},
				{},
			});

			return std::make_shared<SmoothStructval>(SmoothStructval{
				actual_map,
				final_pointer, // todo: is this the actual struct
				true,
			});
		}

		return llvm_to_smooth(unclear_type, extracted);
	} else {
		fprintf(stderr, "Impossible to call a non-map with a symbol, what are you doing?\n");       
		exit(1);
	}
}

// number one rule in this codebase: we never replace a type with its underlying, we keep type informatoin in tact. values respect the types, not the other way around.
// types are not constructed on the fly to match smooth, rather smooth must match underlying types, original types remain in tact.
// underlying types are not determined from smooth, always the other way around.

Smooth evaluate_smooth(
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
		
		Smooth result = evaluate_smooth(igc, merged.types[0]);

		for (size_t i = 1; i < merged.types.size(); i++) {
			Smooth next = evaluate_smooth(igc, merged.types[i]);
			result = merge_smooth(igc, result, next);
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
		
		if (map.leaf_hardval.has_value()) {
			const Hardval& hardval = map.leaf_hardval.value();
			
			std::string type_str = "";
			
			if (map.leaf_type.has_value()) {
				auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&map.leaf_type.value());
				
				if (p_v_rotten) {
					type_str = (*p_v_rotten)->type_str;
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
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			struct_value,
			determine_has_leaf(type),
		});
	}
	
	if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
		const TypePointer& pointer = **p_v_pointer;
		
		if (!pointer.hardval.has_value()) {
			fprintf(stderr, "Evaluation cannot consist of a pointer that has no definitive value, for now (in future, would make sense to deal with pointer of variable, etc.)\n");
			exit(1);
		}
		
		llvm::Value* ptr = evaluate_hardval(igc, pointer.hardval.value());
		
		return std::make_shared<SmoothPointer>(SmoothPointer{
			type,
			ptr,
		});
	}
	
	if (auto p_v_log = std::get_if<std::shared_ptr<TypeLog>>(&type)) {
		const auto& v_log = *p_v_log;
		
		if (v_log->message == nullptr) {
			llvm::Value* log_str = igc.builder.CreateGlobalStringPtr("");
			igc.builder.CreateCall(igc.puts_func, { log_str });
		} else {
			Smooth message_smooth = evaluate_smooth(igc, *v_log->message);
			Smooth leaf_smooth = extract_leaf(igc, message_smooth, true);
			igc.builder.CreateCall(igc.puts_func, { llvm_value(leaf_smooth) });
		}
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, {});
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			struct_value,
			false,
		});
	}
	
	if (auto p_v_log_d = std::get_if<std::shared_ptr<TypeLogD>>(&type)) {
		const auto& v_log_d = *p_v_log_d;

		Smooth message_smooth = evaluate_smooth(igc, *v_log_d->message);

		llvm::Value* leaf;

		if (auto p_v_enum = std::get_if<std::shared_ptr<SmoothEnum>>(&message_smooth)) {
			leaf = (*p_v_enum)->value;
		} else {
			Smooth leaf_smooth = extract_leaf(igc, message_smooth, true);
			leaf = llvm_value(leaf_smooth);
		}

		llvm::Type* leaf_type = leaf->getType();

		uint64_t bit_width = leaf_type->getPrimitiveSizeInBits();

		if (bit_width == 0 && leaf_type->isPointerTy()) {
			bit_width = igc.module.getDataLayout().getPointerSizeInBits();
		}

		if (bit_width == 0) {
			const char* name = std::visit([](auto&& v) { return typeid(*v).name(); }, message_smooth);
			fprintf(stderr, "cannot print this thing because its size is undeterminable\n");
			fprintf(stderr, "message_smooth is %s\n", name);
			exit(1);
		}

		if (leaf_type->isPointerTy()) {
			leaf = igc.builder.CreatePtrToInt(leaf, llvm::IntegerType::get(igc.context, bit_width));
		} else if (!leaf_type->isIntegerTy()) { // interpret the value as raw bits
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
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			struct_value,
			false,
		});
	}
	
	if (auto p_v_log_dd = std::get_if<std::shared_ptr<TypeLogDd>>(&type)) {
		const auto& v_log_dd = *p_v_log_dd;

		Smooth message_smooth = evaluate_smooth(igc, *v_log_dd->message);
		Smooth leaf_smooth = extract_leaf(igc, message_smooth, true);
		llvm::Value* leaf = llvm_value(leaf_smooth);
		llvm::Type* leaf_type = leaf->getType();

		if (!leaf_type->isPointerTy()) {
			fprintf(stderr, "A pointer must be encoded in the leaf when logging in dd mode\n");
			exit(1);
		}

		llvm::Type* i8ptr = llvm::Type::getInt8PtrTy(igc.context);
		llvm::Type* i64 = llvm::Type::getInt64Ty(igc.context);

		llvm::Value* ptr = igc.builder.CreateBitCast(leaf, i8ptr);

		llvm::Value* byte_count;

		if (v_log_dd->is_nullterm) {
			byte_count = llvm::ConstantInt::get(i64, (uint64_t) -1);
		} else {
			Smooth count_smooth = evaluate_smooth(igc, *v_log_dd->byte_count);
			Smooth count_leaf_smooth = extract_leaf(igc, count_smooth, true);
			llvm::Value* count_leaf = llvm_value(count_leaf_smooth);
			
			if (!count_leaf->getType()->isIntegerTy()) {
				fprintf(stderr, "log_dd byte_count must be integeral.\n");
				exit(1);
			}

			byte_count = (count_leaf->getType() == i64)
				? count_leaf
				: igc.builder.CreateZExt(count_leaf, i64);
		}

		igc.builder.CreateCall(igc.log_data_deref_func, { ptr, byte_count });

		llvm::StructType* struct_type = llvm::StructType::get(igc.context, {});
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);

		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			struct_value,
			false,
		});
	}
	
	if (auto p_v_var_walrus = std::get_if<std::shared_ptr<TypeVarWalrus>>(&type)) {
		const auto& v_var_walrus = *p_v_var_walrus;
		
		std::string map_var_name = "m_" + v_var_walrus->name;
		std::string scoped_alloca_name = igc.value_table->scope_id() + "~" + map_var_name;
		
		Smooth smooth = evaluate_smooth(igc, *v_var_walrus->typeval);

		if (is_structwrappable(smooth)) {
			smooth = structwrap(igc, smooth);
		}

		llvm::Value* value = llvm_value(smooth);

		llvm::Value* alloca = igc.builder.CreateAlloca(value->getType(), nullptr, scoped_alloca_name);
		igc.builder.CreateStore(value, alloca);

		ValueSymbolTableEntry entry{
			alloca,
			value->getType(),
			smooth_type(smooth),
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
		
		Smooth smooth = evaluate_smooth(igc, *v_var_assign->typeval);

		llvm::Value* value;

		if (is_structwrappable(smooth)) {
			value = structwrap(igc, smooth)->value;
		} else {
			value = llvm_value(smooth);
		}

		if (!is_subset_type(smooth_type(smooth), existing.type)) {
			fprintf(stderr, "Value is not assignable to variable \"%s\" due to incompatibility (types are not subsets)\n", v_var_assign->name.c_str());
			exit(1);
		}

		igc.builder.CreateStore(value, existing.alloca_ptr);
		
		// as we are using stack allocations, we can mutate via stack address, bypassing immutable registers and phi logic.
		// llvm will optimize this to single-static-assignment (SSA) form for us, when possible.
		
		return access_variable(igc, type);
	}
	
	if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&type)) {
		const auto& v_enum = *p_v_enum;

		if (!v_enum->is_instantiated && !v_enum->hardsym.has_value()) {
			return std::make_shared<SmoothEnum>(SmoothEnum{ type, nullptr });
		}

		uint32_t bit_width = (uint32_t) std::bit_width(v_enum->syms.size() - 1);
		llvm::Type* int_type = llvm::IntegerType::get(igc.context, bit_width);
		
		// if (!v_enum->hardsym.has_value()) {
		// 	fprintf(stderr, "Enum does not have any definitive symbol, could not reduce to one possibility.\n");
		// 	exit(1);
		// }

		const std::string& hardsym = v_enum->hardsym.value();
		auto it = std::find(v_enum->syms.begin(), v_enum->syms.end(), hardsym);

		if (it == v_enum->syms.end()) {
			fprintf(stderr, "While attempting to instantiate \"%s\", it was not found as a possibility.\n", hardsym.c_str());
			exit(1);
		}

		uint32_t enum_idx = (uint32_t) std::distance(v_enum->syms.begin(), it);
		llvm::Value* value = llvm::ConstantInt::get(int_type, enum_idx);
		
		return std::make_shared<SmoothEnum>(SmoothEnum{
			type,
			value,
		});
	}

	if (auto p_v_call_with_sym = std::get_if<std::shared_ptr<TypeCallWithSym>>(&type)) {
		const auto& v_call_with_sym = *p_v_call_with_sym;
		Smooth target_smooth = evaluate_smooth(igc, *v_call_with_sym->target);

		if (!is_structwrappable(target_smooth)) {
			fprintf(stderr, "cannot be expected to call with a sym to a non structish object.\n");
			exit(1);
		}

		auto modern_target_smooth = structwrap(igc, target_smooth);
		
		return access_member(igc, modern_target_smooth, v_call_with_sym->sym);
	}
	
	if (auto p_v_expr_multi = std::get_if<std::shared_ptr<TypeExprMulti>>(&type)) {
		const auto& v_expr_multi = **p_v_expr_multi;
		llvm::Value* result = nullptr;
		bool is_float = false; // we will not allow combining int and float operations implicitely.

		for (const auto& op_numeric : v_expr_multi.ops) {
			Smooth smooth = evaluate_smooth(igc, *op_numeric.operand);
			Smooth value_smooth = extract_leaf(igc, smooth, true);

			if (true
				&& std::get_if<std::shared_ptr<SmoothInt>>(&value_smooth) == nullptr
				&& std::get_if<std::shared_ptr<SmoothFloat>>(&value_smooth) == nullptr
			) {
				fprintf(stderr, "Cannot do multiplication on things that aren't integer or float.\n");
				exit(1);
			}

			bool is_this_one_float = std::get_if<std::shared_ptr<SmoothFloat>>(&value_smooth) != nullptr;
			llvm::Value* value = llvm_value(value_smooth);

			if (result == nullptr) {
				is_float = is_this_one_float;
				result = is_float ? llvm::ConstantFP::get(value->getType(), 1.0) : llvm::ConstantInt::get(value->getType(), 1);
			} else if (is_this_one_float != is_float) {
				fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
				exit(1);
			}

			if (!is_float) { // i don't care about floats for now.
				uint32_t result_bits = result->getType()->getIntegerBitWidth();
				uint32_t value_bits = value->getType()->getIntegerBitWidth();
				
				if (result_bits < value_bits) {
					result = igc.builder.CreateZExt(result, value->getType());
				} else if (value_bits < result_bits) {
					value = igc.builder.CreateZExt(value, result->getType());
				}
			}

			if (op_numeric.op == "*") {
				result = is_float ? igc.builder.CreateFMul(result, value) : igc.builder.CreateMul(result, value);
			} else if (op_numeric.op == "/") {
				result = is_float ? igc.builder.CreateFDiv(result, value) : igc.builder.CreateSDiv(result, value);
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

		if (is_float) {
			return std::make_shared<SmoothFloat>(SmoothFloat{
				type,
				result,
			});
		}
		
		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_addit = std::get_if<std::shared_ptr<TypeExprAddit>>(&type)) {
		const auto& v_expr_addit = **p_v_expr_addit;
		llvm::Value* result = nullptr;
		bool is_float = false;

		for (const auto& op_numeric : v_expr_addit.ops) {
			Smooth smooth = evaluate_smooth(igc, *op_numeric.operand);
			Smooth value_smooth = extract_leaf(igc, smooth, true);

			if (true
				&& std::get_if<std::shared_ptr<SmoothInt>>(&value_smooth) == nullptr
				&& std::get_if<std::shared_ptr<SmoothFloat>>(&value_smooth) == nullptr
			) {
				fprintf(stderr, "Cannot do addition on data that isn't integer or float.\n");
				exit(1);
			}

			bool is_this_one_float = std::get_if<std::shared_ptr<SmoothFloat>>(&value_smooth) != nullptr;
			llvm::Value* value = llvm_value(value_smooth);

			if (result == nullptr) {
				is_float = is_this_one_float;
				result = is_float ? llvm::ConstantFP::get(value->getType(), 0.0) : llvm::ConstantInt::get(value->getType(), 0);
			} else if (is_this_one_float != is_float) {
				fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
				exit(1);
			}

			if (!is_float) { // dont care about floats for now.
				uint32_t result_bits = result->getType()->getIntegerBitWidth();
				uint32_t value_bits = value->getType()->getIntegerBitWidth();
				
				if (result_bits < value_bits) {
					result = igc.builder.CreateZExt(result, value->getType());
				} else if (value_bits < result_bits) {
					value = igc.builder.CreateZExt(value, result->getType());
				}
			}

			if (op_numeric.op == "+") {
				result = is_float ? igc.builder.CreateFAdd(result, value) : igc.builder.CreateAdd(result, value);
			} else if (op_numeric.op == "-") {
				result = is_float ? igc.builder.CreateFSub(result, value) : igc.builder.CreateSub(result, value);
			} else {
				fprintf(stderr, "Some bizarre operator was encountered %s (additive)\n", op_numeric.op.c_str());
				exit(1);
			}
		}

		if (result == nullptr) {
			fprintf(stderr, "additive expression had no operations.\n");
			exit(1);
		}

		if (is_float) {
			return std::make_shared<SmoothFloat>(SmoothFloat{
				type,
				result,
			});
		}
		
		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_logical_and = std::get_if<std::shared_ptr<TypeExprLogicalAnd>>(&type)) {
		const auto& v_expr_logical_and = **p_v_expr_logical_and;

		if (v_expr_logical_and.ops.empty()) {
			fprintf(stderr, "so why did we get no operatands from the frontend.\n");
			exit(1);
		}

		llvm::Type* i1 = llvm::Type::getInt1Ty(igc.context);
		llvm::Value* result = llvm::ConstantInt::get(i1, 1);

		for (const auto& op : v_expr_logical_and.ops) {
			Smooth operand_smooth = evaluate_smooth(igc, *op.operand);

			if (!std::get_if<std::shared_ptr<SmoothEnum>>(&operand_smooth)) {
				fprintf(stderr, "non-enusm not permitted for logical AND.\n");
				exit(1);
			}

			llvm::Value* actual_value = llvm_value(operand_smooth);

			if (!actual_value->getType()->isIntegerTy(1)) {
				fprintf(stderr, "Tried to perform logical AND on something that wasn't intish of size 1.\n");
				exit(1);
			}

			result = igc.builder.CreateAnd(result, actual_value);
		}

		return llvm_to_smooth_bool(igc, result);
	}

	if (auto p_v_expr_logical_or = std::get_if<std::shared_ptr<TypeExprLogicalOr>>(&type)) {
		const auto& v_expr_logical_or = **p_v_expr_logical_or;

		if (v_expr_logical_or.ops.empty()) {
			fprintf(stderr, "some operands missing from thefrontned.\n");
			exit(1);
		}

		llvm::Type* i1 = llvm::Type::getInt1Ty(igc.context);
		llvm::Value* result = llvm::ConstantInt::get(i1, 0);

		for (const auto& op : v_expr_logical_or.ops) {
			Smooth operand_smooth = evaluate_smooth(igc, *op.operand);

			if (!std::get_if<std::shared_ptr<SmoothEnum>>(&operand_smooth)) {
				fprintf(stderr, "non-enum not permitted for logical OR.\n");
				exit(1);
			}

			llvm::Value* actual_value = llvm_value(operand_smooth);

			if (!actual_value->getType()->isIntegerTy(1)) {
				fprintf(stderr, "Tried to perform logical OR on something that wasn't intish of size 1.\n");
				exit(1);
			}

			result = igc.builder.CreateOr(result, actual_value);
		}

		return llvm_to_smooth_bool(igc, result);
	}

	fprintf(stderr, "Unhandled scenario when handling evaluation\n");
	exit(1);
}
