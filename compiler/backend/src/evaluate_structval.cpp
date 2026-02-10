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

SmoothValue evaluate_structval(
	IrGenCtx& igc,
	const Type& type
) {
	if (auto p_v_var_access = std::get_if<std::shared_ptr<TypeVarAccess>>(&type)) {
		const auto& var_access = **p_v_var_access;
		std::string var_name = "m_" + var_access.target_name;
		std::optional<ValueSymbolTableEntry> o_entry = igc.value_table->get(var_name);
		
		if (!o_entry.has_value()) {
			std::string sym_var_name = "ms_:" + var_access.target_name;
			o_entry = igc.value_table->get(sym_var_name);
			
			if (!o_entry.has_value()) {
				fprintf(stderr, "This variable %s was not actually present in our value table\n", var_access.target_name.c_str());
				exit(1);
			}
		}
		
		ValueSymbolTableEntry entry = o_entry.value();
		
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
		
		bool has_leaf = false;
		
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
			has_leaf = true;
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
			has_leaf,
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
	
	fprintf(stderr, "Unhandled scenario when handling evaluation\n");
	exit(1);
}
