#include <cstdio>
#include <cstdlib>
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
	const std::string& target_name
) {
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
		const Type& sym_type = get_underlying_type(unclear_type);
		
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
		return access_variable(igc, (*p_v_var_access)->target_name);
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
		const auto& v_log = std::get<std::shared_ptr<TypeLog>>(type);
		
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
	
	if (auto p_v_assign = std::get_if<std::shared_ptr<TypeAssign>>(&type)) {
		const auto& v_assign = std::get<std::shared_ptr<TypeAssign>>(type);
		
		std::string map_var_name = "m_" + v_assign->name;
		std::string scoped_alloca_name = igc.value_table->scope_id() + "~" + map_var_name;
		
		SmoothValue smooth = evaluate_structval(igc, *v_assign->typeval);
		llvm::Value* alloca = igc.builder.CreateAlloca(smooth.struct_value->getType(), nullptr, scoped_alloca_name);
		igc.builder.CreateStore(smooth.struct_value, alloca);
		
		
		ValueSymbolTableEntry entry{
			alloca,
			smooth.struct_value->getType(),
			smooth.type,
			smooth.has_leaf,
		};
		
		igc.value_table->set(map_var_name, entry);
		
		return access_variable(igc, v_assign->name);
	}
	
	if (auto p_v_call_with_sym = std::get_if<std::shared_ptr<TypeCallWithSym>>(&type)) {
		const auto& v_call_with_sym = std::get<std::shared_ptr<TypeCallWithSym>>(type);
		SmoothValue target_smooth = evaluate_structval(igc, *v_call_with_sym->target);
		return access_member(igc, target_smooth, v_call_with_sym->sym);
	}
	
	fprintf(stderr, "Unhandled scenario when handling evaluation\n");
	exit(1);
}
