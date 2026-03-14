#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <variant>
#include <string>

#include "evaluate_smooth.hpp"
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
#include "build_compare.hpp"
#include "rotten_int_info.hpp"
#include "rotten_float_info.hpp"
#include "extract_map_leaf.hpp"
#include "preinstantiated_smooths.hpp"
#include "access_variable.hpp"
#include "access_member.hpp"
#include "is_type_singletonish.hpp"
#include "evaluate_singletonish.hpp"
#include "produce_call_func.hpp"
#include "happy_smooth.hpp"
#include "create_dummy_igc.hpp"
#include "destroy_dummy_igc.hpp"
#include "better_leaf_agnostically_translate.hpp"
#include "llvm_flexi_type.hpp"
#include "llvm_opaqued_flexi_type.hpp"
#include "t_bundles.hpp"
#include "clone_type_map_for_mutation.hpp"
#include "force_identical_layout.hpp"
#include "preinstantiated_types.hpp"
#include "is_map_leaf_physical.hpp"
#include "fresh_smooth.hpp"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

bool determine_has_leaf(const Type& type) { // soon this function should be deprecated because it is not relevant to non-maps anymore.
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&type)) {
		// return (*p_v_map)->leaf_type.has_value() || (*p_v_map)->leaf_hardval.has_value();
		return (*p_v_map)->leaf_type.has_value();
	}
	
	// what was I thinking here.
	// if (auto p_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
	// 	return true;
	// }
	
	return false;
}

// number one rule in this codebase: we never replace a type with its underlying, we keep type informatoin in tact. values respect the types, not the other way around.
// types are not constructed on the fly to match smooth, rather smooth must match underlying types, original types remain in tact.
// underlying types are not determined from smooth, always the other way around.

Smooth evaluate_smooth(
	std::shared_ptr<IrGenCtx> igc,
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
		
		if (!map.bundle_id.has_value()) {
			fprintf(stderr, "New refactor - Bundles are now required for all maps.\n");
			exit(1);
		}

		Bundle* bundle = igc->toc->bundle_registry->get(map.bundle_id.value());

		if (!bundle) {
			fprintf(stderr, "Bundle not found.");
			exit(1);
		}

		auto p_bundle_map = std::get_if<std::shared_ptr<BundleMap>>(bundle);

		if (!p_bundle_map) {
			fprintf(stderr, "Bundle is not map.");
			exit(1);
		}

		auto bundle_map = *p_bundle_map;

		if (bundle_map->opaque_struct_type == nullptr) {
			bundle_map->opaque_struct_type = llvm::StructType::create(*igc->context);
			bundle_map->opaque_flexi_struct_type = llvm::StructType::create(*igc->context);
		}
		
		std::shared_ptr<ValueSymbolTable> map_value_table = std::make_shared<ValueSymbolTable>(
			create_value_symbol_table(igc->value_table.get())
		);
		
		auto map_igc = std::make_shared<IrGenCtx>(*igc);
		map_igc->value_table = map_value_table;
		// map_igc.block_break = nullptr;
		
		process_map_body(map_igc, map);
		
		std::optional<Smooth> leaf = std::nullopt;
		std::vector<llvm::Type*> member_types;
		std::vector<llvm::Type*> flexi_member_types;
		std::vector<llvm::Value*> member_values;
		std::vector<Smooth> field_smooths;
		
		if (map.leaf_hardval.has_value() && !map.leaf_type.has_value()) {
			fprintf(stderr, "Since refactor, this is a major invariant violation.\n");
			exit(1);
		}
		
		if (map.leaf_type.has_value()) {
			if (is_type_singletonish(map.leaf_type.value())) {
				leaf = evaluate_singletonish(igc, map.leaf_type.value());
			} else {
				leaf = evaluate_smooth(igc, map.leaf_type.value());

				if (map.leaf_hardval.has_value()) {
					// now consistently using the central merging system here for hardvals.
					
					Smooth hardval_smooth = evaluate_hardval(map_igc, map.leaf_hardval.value(), map.leaf_type.value());
					
					leaf = merge_smooth(igc, leaf.value(), hardval_smooth);
				}

				llvm::Value* leaf_value = llvm_value(leaf.value());
				member_types.push_back(leaf_value->getType());
				flexi_member_types.push_back(llvm_opaqued_flexi_type(leaf.value(), igc));
				member_values.push_back(leaf_value);
				field_smooths.push_back(leaf.value());
			}
		}
		
		for (const auto& [sym_name, sym_type] : map.sym_inputs) {
			if (is_type_singletonish(*sym_type)) {
				continue;
			}

			std::string map_sym_var_name = "ms_" + sym_name;
			std::optional<ValueSymbolTableEntry> o_entry = map_igc->value_table->get(map_sym_var_name);
			
			if (!o_entry.has_value()) {
				fprintf(stderr, "Symbol as a variable %s was not really present in the value table\n", sym_name.c_str());
				exit(1);
			}
			
			ValueSymbolTableEntry entry = o_entry.value();
			
			llvm::Value* loaded = map_igc->builder->CreateLoad(
				entry.ir_type,
				entry.alloca_ptr
			);
			
			Smooth sym_smooth = llvm_to_smooth(map_igc, *sym_type, loaded);
			member_types.push_back(loaded->getType());
			flexi_member_types.push_back(llvm_opaqued_flexi_type(sym_smooth, igc));
			member_values.push_back(loaded);
			field_smooths.push_back(sym_smooth);
		}
		
		// llvm will error if setBody called twice.
		bundle_map->opaque_struct_type->setBody(member_types);
		bundle_map->opaque_flexi_struct_type->setBody(flexi_member_types);
		llvm::StructType* struct_type = bundle_map->opaque_struct_type;
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		
		for (size_t i = 0; i < member_values.size(); i++) {
			llvm::Value* forced = force_identical_layout(igc, member_values[i], struct_type->getElementType(i));
			struct_value = igc->builder->CreateInsertValue(struct_value, forced, i);
		}
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			struct_value,
			determine_has_leaf(type),
			leaf,
			[igc, v_map = *p_v_map]() mutable -> llvm::Function* {
				return produce_call_func(igc, v_map);
			},
			[igc, v_map = *p_v_map]() mutable -> llvm::Function* {
				return produce_call_func(igc, v_map, true);
			},
			field_smooths,
		});
	}
	
	if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
		const TypePointer& pointer = **p_v_pointer;
		
		if (!pointer.hardval.has_value()) {
			// fprintf(stderr, "Evaluation cannot consist of a pointer that has no definitive value, for now (in future, would make sense to deal with pointer of variable, etc.)\n");
			// exit(1);
			
			llvm::Value* void_value = smooth_void(igc, type)->value;
			llvm::Type* flexi_type = llvm::PointerType::get(*igc->context, 0);
			
			return std::make_shared<SmoothVoidPointer>(SmoothVoidPointer{
				type,
				flexi_type,
				void_value,
			});
		}
		
		// might be thinking when on earth would a pointer be assigned to a hardval,
		// but don't forget string literals.
		Smooth ptr = evaluate_hardval(igc, pointer.hardval.value(), type);
		
		return std::make_shared<SmoothPointer>(SmoothPointer{
			type,
			llvm_value(ptr),
		});
	}
	
	if (auto p_v_log = std::get_if<std::shared_ptr<TypeLog>>(&type)) {
		const auto& v_log = *p_v_log;
		
		if (v_log->message == nullptr) {
			llvm::Value* log_str = igc->builder->CreateGlobalStringPtr("");
			igc->builder->CreateCall(*igc->puts_func, { log_str });
		} else {
			Smooth message_smooth = evaluate_smooth(igc, *v_log->message);
			Smooth leaf_smooth = extract_leaf(igc, message_smooth, true);
			igc->builder->CreateCall(*igc->puts_func, { llvm_value(leaf_smooth) });
		}
		
		return smooth_void(igc, type);
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
			bit_width = igc->module->getDataLayout().getPointerSizeInBits();
		}

		if (bit_width == 0) {
			llvm::Value* error_str = igc->builder->CreateGlobalStringPtr("[undeterminable size]");
			igc->builder->CreateCall(*igc->puts_func, { error_str });
			
			return smooth_void(igc, type);
		}

		if (leaf_type->isPointerTy()) {
			leaf = igc->builder->CreatePtrToInt(leaf, llvm::IntegerType::get(*igc->context, bit_width));
		} else if (!leaf_type->isIntegerTy()) { // interpret the value as raw bits
			leaf = igc->builder->CreateBitCast(leaf, llvm::IntegerType::get(*igc->context, bit_width));
		}

		uint64_t num_chunks = (bit_width + 63) / 64;
		uint64_t total_bits = num_chunks * 64;

		llvm::Type* i8  = llvm::Type::getInt8Ty(*igc->context);
		llvm::Type* i64 = llvm::Type::getInt64Ty(*igc->context);

		llvm::Value* wide = (total_bits > bit_width) // zero-extend
			? igc->builder->CreateZExt(leaf, llvm::IntegerType::get(*igc->context, total_bits))
			: leaf;

		for (uint64_t i = 0; i < num_chunks; i++) {
			uint64_t shift = (num_chunks - 1 - i) * 64;
			llvm::Value* shifted = igc->builder->CreateLShr(wide, llvm::ConstantInt::get(wide->getType(), shift));
			llvm::Value* chunk = igc->builder->CreateTrunc(shifted, i64);

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

			igc->builder->CreateCall(igc->log_data_func, { chunk, bl, br });
		}

		return smooth_void(igc, type);
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

		llvm::Type* i8ptr = llvm::Type::getInt8PtrTy(*igc->context);
		llvm::Type* i64 = llvm::Type::getInt64Ty(*igc->context);

		llvm::Value* ptr = igc->builder->CreateBitCast(leaf, i8ptr);

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
				: igc->builder->CreateZExt(count_leaf, i64);
		}

		igc->builder->CreateCall(igc->log_data_deref_func, { ptr, byte_count });

		return smooth_void(igc, type);
	}
	
	if (auto p_v_var_walrus = std::get_if<std::shared_ptr<TypeVarWalrus>>(&type)) {
		const auto& v_var_walrus = *p_v_var_walrus;
		
		std::string map_var_name = "m_" + v_var_walrus->name;
		std::string scoped_alloca_name = igc->value_table->scope_id() + "~" + map_var_name;
		
		Smooth smooth = evaluate_smooth(igc, *v_var_walrus->typeval);

		if (is_structwrappable(smooth)) {
			smooth = structwrap(igc, smooth);
		}

		llvm::Value* value = llvm_value(smooth);

		llvm::Value* alloca = igc->builder->CreateAlloca(value->getType(), nullptr, scoped_alloca_name);
		igc->builder->CreateStore(value, alloca);

		ValueSymbolTableEntry entry{
			alloca,
			value->getType(),
			smooth_type(smooth),
			v_var_walrus->is_mut,
			std::holds_alternative<std::shared_ptr<SmoothMapReference>>(smooth)
				? std::optional<Smooth>(smooth)
				: std::nullopt
			,
		};
		
		igc->value_table->set(map_var_name, entry);
		
		return access_variable(igc, type);
	}
	
	if (auto p_v_var_assign = std::get_if<std::shared_ptr<TypeVarAssign>>(&type)) {
		const auto& v_var_assign = *p_v_var_assign;
		
		std::string map_var_name = "m_" + v_var_assign->name;
		std::optional<ValueSymbolTableEntry> o_entry = igc->value_table->get(map_var_name);
		
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

		igc->builder->CreateStore(value, existing.alloca_ptr);
		
		// as we are using stack allocations, we can mutate via stack address, bypassing immutable registers and phi logic.
		// llvm will optimize this to single-static-assignment (SSA) form for us, when possible.
		
		return access_variable(igc, type);
	}

	if (auto p_v_sym_assign = std::get_if<std::shared_ptr<TypeSymAssign>>(&type)) {
		const auto& v_sym_assign = *p_v_sym_assign;

		std::string map_var_name = "m_" + v_sym_assign->name;
		std::optional<ValueSymbolTableEntry> o_entry = igc->value_table->get(map_var_name);

		if (!o_entry.has_value()) {
			fprintf(stderr, "While attempting to assign a symbol on a variable, the variable %s not found.\n", v_sym_assign->name.c_str());
			exit(1);
		}

		const ValueSymbolTableEntry& entry = o_entry.value();

		if (!entry.smooth.has_value()) {
			fprintf(stderr, "Smooth went missing %s.\n", v_sym_assign->name.c_str());
			exit(1);
		}

		auto p_smooth_map_reference = std::get_if<std::shared_ptr<SmoothMapReference>>(&entry.smooth.value());

		if (!p_smooth_map_reference) {
			fprintf(stderr, "Thing isn't a map reference, for now can only mutate map references - wrap it into a reference if you want to mutate it.... %s.\n", v_sym_assign->name.c_str());
			exit(1);
		}

		const auto smooth_map_reference = *p_smooth_map_reference;
		
		auto underlying = get_underlying_type(smooth_map_reference->type);
		
		auto p_type_map_reference = std::get_if<std::shared_ptr<TypeMapReference>>(&underlying);

		if (!p_type_map_reference) {
			fprintf(stderr, "Glitch, was not TypeMapReferecne.\n");
			exit(1);
		}
		
		auto type_map_reference = *p_type_map_reference;

		if (!type_map_reference->is_mutable) {
			fprintf(stderr, "The %s was not mutable, make sure the variable is assigned to a mutable map reference (use &mut ...)\n", v_sym_assign->name.c_str());
			exit(1);
		}

		std::shared_ptr<TypeMap> curr = type_map_reference->target;
		llvm::Value* curr_alloca = smooth_map_reference->value;
		llvm::Type* curr_type = smooth_map_reference->structval_type;

		for (size_t i = 0; i < v_sym_assign->trail.size() - 1; i++) {
			const std::string& segment_of_interest = ":" + v_sym_assign->trail[i];

			if (curr->sym_inputs.find(segment_of_interest) == curr->sym_inputs.end()) {
				fprintf(stderr, "The symbol %s did not exist along the path, when attempting to mutate some variable's data.\n", segment_of_interest.c_str());
				exit(1);
			}
			
			llvm::Value* loaded = igc->builder->CreateLoad(curr_type, curr_alloca);
			Smooth smoothie = llvm_to_smooth(igc, Type(curr), loaded);
			
			auto p_smooth_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smoothie);

			if (!p_smooth_structval) {
				fprintf(stderr, "not struct.\n");
				exit(1);
			}

			Smooth member = access_member(igc, *p_smooth_structval, v_sym_assign->trail[i]);
			auto p_new_reference = std::get_if<std::shared_ptr<SmoothMapReference>>(&member);

			if (!p_new_reference) {
				fprintf(stderr, "not reference.\n");
				exit(1);
			}
			
			auto new_reference = *p_new_reference;

			auto actualunderlying = get_underlying_type(new_reference->type);
			auto p_actualunderlyingreference = std::get_if<std::shared_ptr<TypeMapReference>>(&actualunderlying);

			if (!p_actualunderlyingreference) {
				fprintf(stderr, "not reference again.\n");
				exit(1);
			}

			curr = (*p_actualunderlyingreference)->target;
			curr_alloca = new_reference->value;
			curr_type = new_reference->structval_type;
		}

		const std::string& final_sym = ":" + v_sym_assign->trail.back();
		
		if (curr->sym_inputs.find(final_sym) == curr->sym_inputs.end()) {
			fprintf(stderr, "final %s not found.\n", final_sym.c_str());
			exit(1);
		}

		size_t field_idx = is_map_leaf_physical(curr) ? 1 : 0;

		for (const auto& [sym_name, sym_type] : curr->sym_inputs) { // don't care about performance.
			if (sym_name == final_sym) {
				break;
			}
			
			if (is_type_singletonish(*sym_type)) {
				continue;
			}
			
			field_idx++;
		}

		const Type& type_wanted = Type(*curr->sym_inputs.at(final_sym));

		Smooth spectacular_smooth = evaluate_smooth(igc, *v_sym_assign->typeval);

		if (is_structwrappable(spectacular_smooth)) {
			spectacular_smooth = structwrap(igc, spectacular_smooth);
		}

		spectacular_smooth = happy_smooth(igc, spectacular_smooth, type_wanted, false);
		spectacular_smooth = better_leaf_agnostically_translate(igc, spectacular_smooth, type_wanted, false);
		
		llvm::Value* answer = force_identical_layout(igc, llvm_value(spectacular_smooth), curr_type->getStructElementType((unsigned) field_idx));
		
		llvm::Value* pointer_to_exact_field = igc->builder->CreateStructGEP(curr_type, curr_alloca, (unsigned)field_idx);
		
		igc->builder->CreateStore(answer, pointer_to_exact_field);

		return spectacular_smooth;
	}

	if (auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&type)) {
		llvm::Value* void_value = smooth_void(igc, type)->value;

		if (auto info = rotten_int_info(*p_v_rotten)) {
			llvm::Type* flexi_type = llvm::IntegerType::get(*igc->context, info->bits);
			
			return std::make_shared<SmoothVoidInt>(SmoothVoidInt{
				type,
				flexi_type,
				void_value,
			});
		}

		if (auto info = rotten_float_info(*p_v_rotten)) {
			llvm::Type* flexi_type = nullptr;
			
			if (info->bits == 16) flexi_type = llvm::Type::getHalfTy(*igc->context);
			else if (info->bits == 32) flexi_type = llvm::Type::getFloatTy(*igc->context);
			else if (info->bits == 64) flexi_type = llvm::Type::getDoubleTy(*igc->context);
			else if (info->bits == 128) flexi_type = llvm::Type::getFP128Ty(*igc->context);
			
			return std::make_shared<SmoothVoidFloat>(SmoothVoidFloat{
				type,
				flexi_type,
				void_value,
			});
		}

		fprintf(stderr, "What sort of a rotten that is not intish or floatish is this.");
		exit(1);
	}

	if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&type)) {
		const auto& v_enum = *p_v_enum;

		if (!v_enum->is_instantiated && !v_enum->hardsym.has_value()) {
			return std::make_shared<SmoothEnum>(SmoothEnum{ type, nullptr });
		}

		uint32_t bit_width = (uint32_t) std::bit_width(v_enum->syms.size() - 1);
		llvm::Type* int_type = llvm::IntegerType::get(*igc->context, bit_width);
		
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

		if (auto p_v_map_reference = std::get_if<std::shared_ptr<SmoothMapReference>>(&target_smooth)) {
			const auto& v_map_reference = *p_v_map_reference;
			
			auto underlying_of_actual_interest = get_underlying_type(v_map_reference->type);
			auto p_type_map_reference = std::get_if<std::shared_ptr<TypeMapReference>>(&underlying_of_actual_interest);
			
			if (!p_type_map_reference) {
				fprintf(stderr, "supposed to be map reference type.\n");
				exit(1);
			}
			
			Type target_typeee = Type((*p_type_map_reference)->target);
			auto underlying_of_genuine_interest = get_underlying_type(target_typeee);
			
			llvm::Value* loaded = igc->builder->CreateLoad(v_map_reference->structval_type, v_map_reference->value);
			
			Smooth loaded_smooth = llvm_to_smooth(igc, underlying_of_genuine_interest, loaded);
			
			// if we load entire struct , llvm optimizer should be able to convert this to just grabbing the memory address offset beforehand for the desired member.
			
			auto actual_target = std::get<std::shared_ptr<SmoothStructval>>(loaded_smooth);

			Smooth member = access_member(igc, actual_target, v_call_with_sym->sym);
			
			if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&member)) {
				(*p_v_structval)->intended_this = v_map_reference->value;
			}
			
			return member;
		}

		if (!is_structwrappable(target_smooth)) {
			fprintf(stderr, "cannot be expected to call with a sym to a non structish object.\n");
			exit(1);
		}

		auto modern_target_smooth = structwrap(igc, target_smooth);
		auto is_it_map = std::get_if<std::shared_ptr<TypeMap>>(&modern_target_smooth->type);

		if (!is_it_map) {
			fprintf(stderr, "bizarre situaiothpq!!\n");
			exit(1);
		}

		llvm::Value* converting_to_reference = igc->builder->CreateAlloca(modern_target_smooth->value->getType(), nullptr, "sym_this_ref");
		igc->builder->CreateStore(modern_target_smooth->value, converting_to_reference);

		auto new_reference = std::make_shared<TypeMapReference>();
		new_reference->target = *is_it_map;
		new_reference->is_mutable = (*is_it_map)->is_this_mutable;

		Smooth member = access_member(igc, modern_target_smooth, v_call_with_sym->sym);

		if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&member)) {
			(*p_v_structval)->intended_this = converting_to_reference;
		}

		return member;
	}

	if (auto p_v_call_with_dynamic = std::get_if<std::shared_ptr<TypeCallWithDynamic>>(&type)) {
		const auto& v_call_with_dynamic = *p_v_call_with_dynamic;
		
		Smooth target_smooth = evaluate_smooth(igc, *v_call_with_dynamic->target);
		Smooth data_smooth = evaluate_smooth(igc, *v_call_with_dynamic->call_data);

		if (auto p_v_map_reference = std::get_if<std::shared_ptr<SmoothMapReference>>(&target_smooth)) {
			const auto& v_map_reference = *p_v_map_reference;

			auto actual_target_underlying = get_underlying_type(v_map_reference->type);
			auto p_type_map_reference = std::get_if<std::shared_ptr<TypeMapReference>>(&actual_target_underlying);
			
			if (!p_type_map_reference) {
				fprintf(stderr, "supposed to be map reference type.\n");
				exit(1);
			}
			
			Type target_typeee = Type((*p_type_map_reference)->target);
			auto underlying_of_genuine_interest = get_underlying_type(target_typeee);
			auto actual_target_type = std::get_if<std::shared_ptr<TypeMap>>(&underlying_of_genuine_interest);
			
			if (!actual_target_type || (*actual_target_type)->call_output_type == nullptr || (*actual_target_type)->call_input_type == nullptr) {
				fprintf(stderr, "somehow was not an actual map stored here - bug.\n");
				exit(1);
			}

			if (auto p_smooth_enum = std::get_if<std::shared_ptr<SmoothEnum>>(&data_smooth)) {
				const auto& smooth_enum = *p_smooth_enum;
				auto p_type_enum = std::get_if<std::shared_ptr<TypeEnum>>(&smooth_enum->type);

				if (!p_type_enum || !(*p_type_enum)->hardsym.has_value()) {
					fprintf(stderr, "only hardsym implemented for now.\n");
					exit(1);
				}

				const std::string& sym = (*p_type_enum)->hardsym.value();

				llvm::Value* loaded = igc->builder->CreateLoad(v_map_reference->structval_type, v_map_reference->value);
				Smooth loaded_smooth = llvm_to_smooth(igc, underlying_of_genuine_interest, loaded);
				
				auto resulting_smooth = std::get<std::shared_ptr<SmoothStructval>>(loaded_smooth);
				
				return access_member(igc, resulting_smooth, sym);
			} else if (std::get_if<std::shared_ptr<SmoothStructval>>(&data_smooth)) {
				{ // this hack better fix the opaque type error.
					DummyIgc dummy = create_dummy_igc(igc);
					evaluate_smooth(dummy.igc, Type((*actual_target_type)->call_input_type));
					destroy_dummy_igc(dummy);
				}

				Smooth wrapped_up = structwrap(igc, data_smooth);
				Smooth upgraded = happy_smooth(igc, wrapped_up, Type((*actual_target_type)->call_input_type), true);

				llvm::Function* call_func = produce_call_func(igc, *actual_target_type);
				llvm::Function* call_func_alwaysinline = produce_call_func(igc, *actual_target_type, true);

				Smooth translated = better_leaf_agnostically_translate(igc, upgraded, Type((*actual_target_type)->call_input_type), true);
				llvm::StructType* the_type_it_should_be = llvm::cast<llvm::StructType>(call_func->getFunctionType()->getParamType(2));
				llvm::Value* input_payload = force_identical_layout(igc, llvm_value(translated), the_type_it_should_be);
				
				llvm::Function* optimized_func = (v_call_with_dynamic->is_flag_alwaysinline && call_func_alwaysinline != nullptr)
					? call_func_alwaysinline
					: call_func;

				llvm::Type* opaque_pointer = llvm::PointerType::get(*igc->context, 0);
				
				// todo: in future we might need to conditionally drop these parameters, it depends whether optimizer is smart enough. actually i think optimizer smart enough provided struct size is zero and only use is conditionally applied "sizeof".
				std::optional<ValueSymbolTableEntry> o_entry_mod = igc->value_table->get("m_mod");
				
				if (!o_entry_mod.has_value()) {
					fprintf(stderr, "mod context not present!!\n");
					exit(1);
				}
				
				llvm::Value* arg_mod = igc->builder->CreateLoad(opaque_pointer, o_entry_mod->alloca_ptr);

				llvm::Value* arg_this = igc->builder->CreatePointerCast(v_map_reference->value, opaque_pointer);

				llvm::CallInst* output_value = igc->builder->CreateCall(optimized_func, {
					arg_mod,
					arg_this,
					input_payload,
				});

				if (v_call_with_dynamic->is_flag_alwaysinline) {
					output_value->addFnAttr(llvm::Attribute::AlwaysInline);
				}

				Type output_type = Type((*actual_target_type)->call_output_predicted_type);

				return llvm_to_smooth(igc, output_type, output_value);
			}
		}

		if (!is_structwrappable(target_smooth)) {
			fprintf(stderr, "have to call on a structish thing.\n");
			exit(1);
		}

		if (auto p_smooth_enum = std::get_if<std::shared_ptr<SmoothEnum>>(&data_smooth)) {
			const auto& smooth_enum = *p_smooth_enum;
			auto p_type_enum = std::get_if<std::shared_ptr<TypeEnum>>(&smooth_enum->type);

			// for now we cannot allow dynamic field selection because type is too variable.
			// i want to solve this down the road using comptime logic where the compiler knows the set of possible types, but one must match the input sym to determine which type is in effect.
			// each match statement would constraint the possibility and prove to the compiler what the type is.
			
			if (!p_type_enum || !(*p_type_enum)->hardsym.has_value()) {
				fprintf(stderr, "for now : hardsym must be statically provided for dynamic sym call. will be tackled down the road using my so-called \"description\" system.\n");
				exit(1);
			}

			const std::string& sym = (*p_type_enum)->hardsym.value();
			
			auto improved_smooth = structwrap(igc, target_smooth);
			
			return access_member(igc, improved_smooth, sym);
		} else if (std::get_if<std::shared_ptr<SmoothStructval>>(&data_smooth)) {
			auto actual_target = std::get_if<std::shared_ptr<SmoothStructval>>(&target_smooth);

			if (!actual_target || !(*actual_target)->call_func) {
				fprintf(stderr, "cannot do a map call on a target that doesn't support it. - map is not map-callable.");
				exit(1);
			}

			auto actual_target_underlying = get_underlying_type((*actual_target)->type);
			auto actual_target_type = std::get_if<std::shared_ptr<TypeMap>>(&actual_target_underlying);
			
			if (!actual_target_type || (*actual_target_type)->call_output_type == nullptr || (*actual_target_type)->call_input_type == nullptr) {
				fprintf(stderr, "somehow target had no appropriate map information.\n");
				exit(1);
			}

			{ // this hack better fix the opaque type error.
				DummyIgc dummy = create_dummy_igc(igc);
				evaluate_smooth(dummy.igc, Type((*actual_target_type)->call_input_type));
				destroy_dummy_igc(dummy);
			}

			Smooth wrapped_up = structwrap(igc, data_smooth);
			Smooth upgraded = happy_smooth(igc, wrapped_up, Type((*actual_target_type)->call_input_type), true);
			
			llvm::Function* my_call_func = (*actual_target)->call_func();
			llvm::Function* my_call_func_alwaysinline = (*actual_target)->call_func_alwaysinline ? (*actual_target)->call_func_alwaysinline() : nullptr;
			
			Smooth translated = better_leaf_agnostically_translate(igc, upgraded, Type((*actual_target_type)->call_input_type), true);
			llvm::StructType* the_type_it_should_be = llvm::cast<llvm::StructType>(my_call_func->getFunctionType()->getParamType(2));
			llvm::Value* input_payload = force_identical_layout(igc, llvm_value(translated), the_type_it_should_be);
			
			llvm::Function* optimized_func = (v_call_with_dynamic->is_flag_alwaysinline && my_call_func_alwaysinline != nullptr)
				? my_call_func_alwaysinline
				: my_call_func;

			llvm::Type* opaque_pointer = llvm::PointerType::get(*igc->context, 0);

			std::optional<ValueSymbolTableEntry> o_entry_mod = igc->value_table->get("m_mod");

			if (!o_entry_mod.has_value()) {
				fprintf(stderr, "mod context not present!!\n");
				exit(1);
			}

			llvm::Value* arg_mod = igc->builder->CreateLoad(opaque_pointer, o_entry_mod->alloca_ptr);

			llvm::Value* arg_this = [&]() -> llvm::Value* {
				if ((*actual_target)->intended_this.has_value()) {
					return igc->builder->CreateBitOrPointerCast((*actual_target)->intended_this.value(), opaque_pointer);
				}
				llvm::Value* reference_version = igc->builder->CreateAlloca((*actual_target)->value->getType(), nullptr);
				igc->builder->CreateStore((*actual_target)->value, reference_version);
				return igc->builder->CreatePointerCast(reference_version, opaque_pointer);
			}();

			llvm::CallInst* output_value = igc->builder->CreateCall(optimized_func, {
				arg_mod,
				arg_this,
				input_payload,
			});

			if (v_call_with_dynamic->is_flag_alwaysinline) {
				output_value->addFnAttr(llvm::Attribute::AlwaysInline);
			}

			Type output_type = Type((*actual_target_type)->call_output_predicted_type);

			return llvm_to_smooth(igc, output_type, output_value);
		} else {
			fprintf(stderr, "currenty only sym calls and map calls are supported.\n");
			exit(1);
		}
	}
	
	if (auto p_v_expr_multi = std::get_if<std::shared_ptr<TypeExprMulti>>(&type)) {
		const auto& v_expr_multi = **p_v_expr_multi;
		llvm::Value* result = nullptr;
		std::optional<bool> is_float; // we will not allow combining int and float operations implicitely.
		char prefix = 'i';

		for (const auto& op_numeric : v_expr_multi.ops) {
			Smooth smooth = evaluate_smooth(igc, *op_numeric.operand);
			Smooth value_smooth = extract_leaf(igc, smooth, true);

			Type operand_underlying = get_underlying_type(extract_map_leaf(*op_numeric.operand, true));
			auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&operand_underlying);

			if (!p_v_rotten) {
				fprintf(stderr, "cannot multiply something non-rotten\n");
				fprintf(stderr, "Cannot do multiplication on things that aren't integer or float.\n");
				exit(1);
			}

			bool is_this_one_float;

			if (auto info = rotten_int_info(*p_v_rotten)) {
				if (is_float.has_value() && is_float.value()) {
					fprintf(stderr, "do not mix int and float like this (got int after float).\n");
					fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
					exit(1);
				}
				
				is_float = false;
				prefix = info->prefix;
				is_this_one_float = false;
			} else if (auto info = rotten_float_info(*p_v_rotten)) {
				if (is_float.has_value() && !is_float.value()) {
					fprintf(stderr, "do not mix int and float (got float after int)\n");
					fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
					exit(1);
				}
				
				is_float = true;
				is_this_one_float = true;
			} else {
				fprintf(stderr, "some other rotten type was used that is not numerical (not supporting multiplication)\n");
				exit(1);
			}

			llvm::Value* value = llvm_value(value_smooth);

			if (result == nullptr) {
				result = is_float.value() ? llvm::ConstantFP::get(value->getType(), 1.0) : llvm::ConstantInt::get(value->getType(), 1);
			}

			if (!is_float.value()) { // i don't care about floats for now.
				uint32_t result_bits = result->getType()->getIntegerBitWidth();
				uint32_t value_bits = value->getType()->getIntegerBitWidth();
				
				if (result_bits < value_bits) {
					result = igc->builder->CreateZExt(result, value->getType());
				} else if (value_bits < result_bits) {
					value = igc->builder->CreateZExt(value, result->getType());
				}
			}

			if (op_numeric.op == "*") {
				result = is_float.value() ? igc->builder->CreateFMul(result, value) : igc->builder->CreateMul(result, value);
			} else if (op_numeric.op == "/") {
				result = is_float.value() ? igc->builder->CreateFDiv(result, value) : igc->builder->CreateSDiv(result, value);
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

		uint32_t actual_bits = result->getType()->getPrimitiveSizeInBits();
		auto rotten_outcome = std::make_shared<TypeRotten>();

		if (is_float.value()) {
			rotten_outcome->type_str = "f" + std::to_string(actual_bits);
		} else {
			rotten_outcome->type_str = std::string(1, prefix) + std::to_string(actual_bits);
		}

		(*p_v_expr_multi)->underlying_type = std::make_shared<Type>(Type(rotten_outcome));

		if (is_float.value()) {
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
		std::optional<bool> is_float;
		char prefix = 'i';

		for (const auto& op_numeric : v_expr_addit.ops) {
			Smooth smooth = evaluate_smooth(igc, *op_numeric.operand);
			Smooth value_smooth = extract_leaf(igc, smooth, true);

			Type operand_underlying = get_underlying_type(extract_map_leaf(*op_numeric.operand, true));
			auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&operand_underlying);

			if (!p_v_rotten) {
				fprintf(stderr, "cannot add something non-rotten\n");
				fprintf(stderr, "Cannot do addition on data that isn't integer or float.\n");
				exit(1);
			}

			bool is_this_one_float;

			if (auto info = rotten_int_info(*p_v_rotten)) {
				if (is_float.has_value() && is_float.value()) {
					fprintf(stderr, "do not mix int and float like this (got int after float).\n");
					fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
					exit(1);
				}
				
				is_float = false;
				prefix = info->prefix;
				is_this_one_float = false;
			} else if (auto info = rotten_float_info(*p_v_rotten)) {
				if (is_float.has_value() && !is_float.value()) {
					fprintf(stderr, "do not mix int and float (got float after int)\n");
					fprintf(stderr, "The compiler does not support implicit combining of ints and float operations.\n");
					exit(1);
				}
				
				is_float = true;
				is_this_one_float = true;
			} else {
				fprintf(stderr, "some other rotten type was used that is not numerical (not supporting addition)\n");
				exit(1);
			}
			
			llvm::Value* value = llvm_value(value_smooth);

			if (result == nullptr) {
				result = is_float.value() ? llvm::ConstantFP::get(value->getType(), 0.0) : llvm::ConstantInt::get(value->getType(), 0);
			}

			if (!is_float.value()) { // dont care about floats for now.
				uint32_t result_bits = result->getType()->getIntegerBitWidth();
				uint32_t value_bits = value->getType()->getIntegerBitWidth();
				
				if (result_bits < value_bits) {
					result = igc->builder->CreateZExt(result, value->getType());
				} else if (value_bits < result_bits) {
					value = igc->builder->CreateZExt(value, result->getType());
				}
			}

			if (op_numeric.op == "+") {
				result = is_float.value() ? igc->builder->CreateFAdd(result, value) : igc->builder->CreateAdd(result, value);
			} else if (op_numeric.op == "-") {
				result = is_float.value() ? igc->builder->CreateFSub(result, value) : igc->builder->CreateSub(result, value);
			} else {
				fprintf(stderr, "Some bizarre operator was encountered %s (additive)\n", op_numeric.op.c_str());
				exit(1);
			}
		}

		if (result == nullptr) {
			fprintf(stderr, "additive expression had no operations.\n");
			exit(1);
		}

		uint32_t actual_bits = result->getType()->getPrimitiveSizeInBits();
		auto rotten_outcome = std::make_shared<TypeRotten>();

		if (is_float.value()) {
			rotten_outcome->type_str = "f" + std::to_string(actual_bits);
		} else {
			rotten_outcome->type_str = std::string(1, prefix) + std::to_string(actual_bits);
		}

		(*p_v_expr_addit)->underlying_type = std::make_shared<Type>(Type(rotten_outcome));

		if (is_float.value()) {
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

	if (auto p_v_expr_modulo = std::get_if<std::shared_ptr<TypeExprModulo>>(&type)) {
		const auto v_expr_modulo = *p_v_expr_modulo;

		Smooth smooth_a = evaluate_smooth(igc, *v_expr_modulo->operand_a);
		llvm::Value* value_a = llvm_value(extract_leaf(igc, smooth_a, true));
		
		Smooth smooth_b = evaluate_smooth(igc, *v_expr_modulo->operand_b);
		llvm::Value* value_b = llvm_value(extract_leaf(igc, smooth_b, true));

		uint32_t a_bits = value_a->getType()->getIntegerBitWidth();
		uint32_t b_bits = value_b->getType()->getIntegerBitWidth();

		if (a_bits < b_bits) {
			value_a = igc->builder->CreateZExt(value_a, value_b->getType());
		} else if (b_bits < a_bits) {
			value_b = igc->builder->CreateZExt(value_b, value_a->getType());
		}

		llvm::Value* result;

		result = igc->builder->CreateURem(value_a, value_b);

		uint32_t actual_bits = result->getType()->getPrimitiveSizeInBits();
		auto improved_rotten = std::make_shared<TypeRotten>();
		improved_rotten->type_str = "u" + std::to_string(actual_bits);
		(*p_v_expr_modulo)->underlying_type = std::make_shared<Type>(Type(improved_rotten));

		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_shift_right = std::get_if<std::shared_ptr<TypeExprShiftRight>>(&type)) {
		const auto v_expr_shift_right = *p_v_expr_shift_right;

		Smooth smooth_a = evaluate_smooth(igc, *v_expr_shift_right->operand_a);
		llvm::Value* value_a = llvm_value(extract_leaf(igc, smooth_a, true));
		
		Smooth smooth_b = evaluate_smooth(igc, *v_expr_shift_right->operand_b);
		llvm::Value* value_b = llvm_value(extract_leaf(igc, smooth_b, true));

		uint32_t a_bits = value_a->getType()->getIntegerBitWidth();
		uint32_t b_bits = value_b->getType()->getIntegerBitWidth();

		if (a_bits < b_bits) {
			value_a = igc->builder->CreateZExt(value_a, value_b->getType());
		} else if (b_bits < a_bits) {
			value_b = igc->builder->CreateZExt(value_b, value_a->getType());
		}

		llvm::Value* result = igc->builder->CreateLShr(value_a, value_b);

		auto cool_rotten = std::make_shared<TypeRotten>();
		cool_rotten->type_str = "u" + std::to_string(result->getType()->getIntegerBitWidth());
		(*p_v_expr_shift_right)->underlying_type = std::make_shared<Type>(Type(cool_rotten));

		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_shift_left = std::get_if<std::shared_ptr<TypeExprShiftLeft>>(&type)) {
		const auto v_expr_shift_left = *p_v_expr_shift_left;

		Smooth smooth_a = evaluate_smooth(igc, *v_expr_shift_left->operand_a);
		llvm::Value* value_a = llvm_value(extract_leaf(igc, smooth_a, true));
		
		Smooth smooth_b = evaluate_smooth(igc, *v_expr_shift_left->operand_b);
		llvm::Value* value_b = llvm_value(extract_leaf(igc, smooth_b, true));
		
		uint32_t a_bits = value_a->getType()->getIntegerBitWidth();
		uint32_t b_bits = value_b->getType()->getIntegerBitWidth();

		if (a_bits < b_bits) {
			value_a = igc->builder->CreateZExt(value_a, value_b->getType());
		} else if (b_bits < a_bits) {
			value_b = igc->builder->CreateZExt(value_b, value_a->getType());
		}

		llvm::Value* result = igc->builder->CreateShl(value_a, value_b);

		auto generated_rotten = std::make_shared<TypeRotten>();
		generated_rotten->type_str = "u" + std::to_string(result->getType()->getIntegerBitWidth());
		(*p_v_expr_shift_left)->underlying_type = std::make_shared<Type>(Type(generated_rotten));

		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_bitwise_and = std::get_if<std::shared_ptr<TypeExprBitwiseAnd>>(&type)) {
		const auto v_expr_bitwise_and = *p_v_expr_bitwise_and;
		
		llvm::Value* result = nullptr;

		for (const auto& operand : v_expr_bitwise_and->operands) {
			Smooth smooth = evaluate_smooth(igc, *operand);
			Smooth leaf = extract_leaf(igc, smooth, true);

			llvm::Value* value = llvm_value(leaf);

			if (result == nullptr) {
				result = value;
				continue;
			}

			uint32_t result_bits = result->getType()->getIntegerBitWidth();
			uint32_t value_bits = value->getType()->getIntegerBitWidth();

			if (result_bits < value_bits) {
				result = igc->builder->CreateZExt(result, value->getType());
			} else if (value_bits < result_bits) {
				value = igc->builder->CreateZExt(value, result->getType());
			}

			result = igc->builder->CreateAnd(result, value);
		}

		if (result == nullptr) {
			fprintf(stderr, "no operations actually given\n");
			exit(1);
		}

		uint32_t actual_bits = result->getType()->getPrimitiveSizeInBits();
		auto new_rotten = std::make_shared<TypeRotten>();
		new_rotten->type_str = "u" + std::to_string(actual_bits);
		(*p_v_expr_bitwise_and)->underlying_type = std::make_shared<Type>(Type(new_rotten));

		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_bitwise_or = std::get_if<std::shared_ptr<TypeExprBitwiseOr>>(&type)) {
		const auto v_expr_bitwise_or = *p_v_expr_bitwise_or;
		
		llvm::Value* result = nullptr;

		for (const auto& operand : v_expr_bitwise_or->operands) {
			Smooth smooth = evaluate_smooth(igc, *operand);
			Smooth leaf = extract_leaf(igc, smooth, true);

			llvm::Value* value = llvm_value(leaf);

			if (result == nullptr) {
				result = value;
				continue;
			}

			uint32_t result_bits = result->getType()->getIntegerBitWidth();
			uint32_t value_bits = value->getType()->getIntegerBitWidth();

			if (result_bits < value_bits) {
				result = igc->builder->CreateZExt(result, value->getType());
			} else if (value_bits < result_bits) {
				value = igc->builder->CreateZExt(value, result->getType());
			}

			result = igc->builder->CreateOr(result, value);
		}

		if (result == nullptr) {
			fprintf(stderr, "no operations actually given\n");
			exit(1);
		}

		uint32_t actual_bits = result->getType()->getPrimitiveSizeInBits();
		auto new_rotten = std::make_shared<TypeRotten>();
		new_rotten->type_str = "u" + std::to_string(actual_bits);
		(*p_v_expr_bitwise_or)->underlying_type = std::make_shared<Type>(Type(new_rotten));

		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			result,
		});
	}

	if (auto p_v_expr_not = std::get_if<std::shared_ptr<TypeExprNot>>(&type)) {
		const auto& v_expr_not = *p_v_expr_not;

		Smooth smooth = evaluate_smooth(igc, *v_expr_not->operand);
		
		auto p_v_enum = std::get_if<std::shared_ptr<SmoothEnum>>(&smooth);
		
		if (!p_v_enum) {
			fprintf(stderr, "Must use not (!) on a boolean! ENUM\n");
			exit(1);
		}
		
		llvm::Value* value = llvm_value(smooth);

		llvm::Value* result = igc->builder->CreateNot(value);

		(*p_v_expr_not)->underlying_type = std::make_shared<Type>(Type(type_bool));
		
		return llvm_to_smooth_bool(igc, result);
	}

	if (auto p_v_compare = std::get_if<std::shared_ptr<TypeCompare>>(&type)) {
		const auto& v_compare = **p_v_compare;

		Smooth a_smooth = evaluate_smooth(igc, *v_compare.operand_a);
		llvm::Value* a = llvm_value(a_smooth);

		Smooth b_smooth = evaluate_smooth(igc, *v_compare.operand_b);
		llvm::Value* b = llvm_value(b_smooth);

		llvm::Value* cmp = build_compare(igc, a, b, v_compare.operator_);

		return llvm_to_smooth_bool(igc, cmp);
	}

	if (auto p_v_expr_logical_and = std::get_if<std::shared_ptr<TypeExprLogicalAnd>>(&type)) {
		const auto& v_expr_logical_and = **p_v_expr_logical_and;

		if (v_expr_logical_and.ops.empty()) {
			fprintf(stderr, "so why did we get no operatands from the frontend.\n");
			exit(1);
		}

		llvm::Type* i1 = llvm::Type::getInt1Ty(*igc->context);
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

			result = igc->builder->CreateAnd(result, actual_value);
		}

		return llvm_to_smooth_bool(igc, result);
	}

	if (auto p_v_expr_logical_or = std::get_if<std::shared_ptr<TypeExprLogicalOr>>(&type)) {
		const auto& v_expr_logical_or = **p_v_expr_logical_or;

		if (v_expr_logical_or.ops.empty()) {
			fprintf(stderr, "some operands missing from thefrontned.\n");
			exit(1);
		}

		llvm::Type* i1 = llvm::Type::getInt1Ty(*igc->context);
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

			result = igc->builder->CreateOr(result, actual_value);
		}

		return llvm_to_smooth_bool(igc, result);
	}

	if (std::get_if<std::shared_ptr<TypeVoid>>(&type)) {
		return smooth_void(igc, type);
	}

	if (auto p_v_extern_ccc = std::get_if<std::shared_ptr<TypeExternCcc>>(&type)) {
		const auto& v_extern_ccc = **p_v_extern_ccc;

		std::vector<llvm::Type*> c_abi_parameter_types;
		std::vector<std::string> c_abi_parameter_names;

		// note that leaves are not to form part of the C abi.
		// if the user wants to use the leaf, they must store it in an actual sym.
		
		for (const auto& [sym_name, p_sym_type] : v_extern_ccc.call_input_type->sym_inputs) {
			if (is_type_singletonish(*p_sym_type)) {
				continue;
			}

			DummyIgc dummy = create_dummy_igc(igc);
			Smooth sym_smooth = evaluate_smooth(dummy.igc, *p_sym_type);
			
			if (is_structwrappable(sym_smooth)) {
				Smooth wrapped = structwrap(dummy.igc, sym_smooth);
				Smooth leaf = extract_leaf(dummy.igc, wrapped);
				sym_smooth = leaf;
			}
			
			llvm::Type* type_in_context_of_function_call = llvm_flexi_type(sym_smooth);
			destroy_dummy_igc(dummy);

			c_abi_parameter_types.push_back(type_in_context_of_function_call);
			c_abi_parameter_names.push_back(sym_name);
		}

		// this is used for determine what to do with the wrapper function, not the actual extern function.
		
		DummyIgc dummy2 = create_dummy_igc(igc);
		Smooth input_probe = evaluate_smooth(dummy2.igc, Type(v_extern_ccc.call_input_type));
		llvm::StructType* input_struct_type = llvm::cast<llvm::StructType>(llvm_opaqued_flexi_type(input_probe, dummy2.igc));
		destroy_dummy_igc(dummy2);

		const auto& output_sym_inputs = v_extern_ccc.call_output_type->sym_inputs;
		const bool is_return_value_actually_present = !output_sym_inputs.empty();

		llvm::Type* c_abi_return_type = [&]() -> llvm::Type* {
			if (!is_return_value_actually_present) {
				return llvm::Type::getVoidTy(*igc->context);
			}

			const auto& first_field = output_sym_inputs.begin()->second;

			DummyIgc dummy = create_dummy_igc(igc);
			Smooth field_smooth = evaluate_smooth(dummy.igc, *first_field);
			llvm::Type* result = llvm_flexi_type(field_smooth);
			destroy_dummy_igc(dummy);

			return result;
		}();

		llvm::StructType* output_struct_type = [&]() -> llvm::StructType* {
			if (!is_return_value_actually_present) {
				return llvm::StructType::get(*igc->context, llvm::ArrayRef<llvm::Type*>{});
			}

			return llvm::StructType::get(*igc->context, llvm::ArrayRef<llvm::Type*>{ c_abi_return_type });
		}();

		llvm::FunctionType* the_extern_type = llvm::FunctionType::get(
			c_abi_return_type,
			c_abi_parameter_types,
			false
		);

		llvm::FunctionCallee the_extern_callee = igc->module->getOrInsertFunction(
			v_extern_ccc.function_name,
			the_extern_type
		);
		
		auto* function_handle = llvm::dyn_cast<llvm::Function>(the_extern_callee.getCallee());

		if (!function_handle) {
			fprintf(stderr, "glitch\n");
			exit(1);
		}
		
		function_handle->setCallingConv(llvm::CallingConv::C);

		// as these are dropped we can keep them opaque.
		llvm::Type* opaque_pointer = llvm::PointerType::get(*igc->context, 0);

		llvm::FunctionType* wrapper_function_type = llvm::FunctionType::get(
			output_struct_type,
			{ opaque_pointer, opaque_pointer, input_struct_type },
			false
		);

		static int next_extern_ccc_id = 0;

		auto produce_wrapper_func = [&](bool is_alwaysinline) -> llvm::Function* {
			std::string wrapper_name = "__ec_extern_ccc_" + std::to_string(next_extern_ccc_id++);

			llvm::Function* wrapper_function = llvm::Function::Create(
				wrapper_function_type,
				llvm::Function::PrivateLinkage,
				wrapper_name,
				*igc->module
			);

			wrapper_function->addFnAttr(llvm::Attribute::AlwaysInline);

			llvm::BasicBlock* wrapper_entry = llvm::BasicBlock::Create(*igc->context, "entry", wrapper_function);
			auto wrapper_builder = std::make_shared<llvm::IRBuilder<>>(*igc->context);
			wrapper_builder->SetInsertPoint(wrapper_entry);
			
			llvm::Argument* lonely_input_argument = wrapper_function->getArg(2);
			lonely_input_argument->setName("input_struct");

			std::vector<llvm::Value*> arg_extraction;

			unsigned field_index = 0;
			unsigned c_abi_index = 0;

			auto igc_actual_call = std::make_shared<IrGenCtx>(IrGenCtx{
				igc->context,
				igc->module,
				wrapper_builder,
				std::make_shared<ValueSymbolTable>(create_value_symbol_table()),
				igc->puts_func,
				igc->log_data_func,
				igc->log_data_deref_func,
				nullptr,
				igc->toc,
			});

			for (const auto& [sym_name, p_sym_type] : v_extern_ccc.call_input_type->sym_inputs) {
				if (is_type_singletonish(*p_sym_type)) {
					continue;
				}

				llvm::Value* extracted = wrapper_builder->CreateExtractValue(lonely_input_argument, field_index);

				Smooth field_smooth = llvm_to_smooth(igc_actual_call, *p_sym_type, extracted);
				Smooth wrapped_smooth = structwrap(igc_actual_call, field_smooth);
				Smooth leaf_smooth = extract_leaf(igc_actual_call, wrapped_smooth);

				arg_extraction.push_back(force_identical_layout(igc_actual_call, llvm_value(leaf_smooth), c_abi_parameter_types[c_abi_index]));

				field_index++;
				c_abi_index++;
			}
			
			llvm::CallInst* call_to_extern = wrapper_builder->CreateCall(the_extern_callee, arg_extraction);

			if (is_alwaysinline) {
				call_to_extern->addFnAttr(llvm::Attribute::AlwaysInline);
			}

			llvm::Value* final_output = [&]() -> llvm::Value* {
				if (!is_return_value_actually_present) {
					return llvm::UndefValue::get(output_struct_type);
				}

				llvm::Value* wrapped_up_in_a_tortilla = llvm::UndefValue::get(output_struct_type);
				
				return wrapper_builder->CreateInsertValue(wrapped_up_in_a_tortilla, call_to_extern, 0);
			}();
			
			wrapper_builder->CreateRet(final_output);

			return wrapper_function;
		};

		llvm::Function* wrapper_traditional = produce_wrapper_func(false);
		llvm::Function* wrapper_alwaysinline = produce_wrapper_func(true);

		auto bundle_map = std::make_shared<BundleMap>();
		
		bundle_map->call_func = wrapper_traditional;
		bundle_map->call_func_alwaysinline = wrapper_alwaysinline;
		
		uint64_t bundle_id = igc->toc->bundle_registry->install(Bundle(bundle_map));

		auto callable = std::make_shared<TypeMap>();
		
		callable->call_input_type = v_extern_ccc.call_input_type;
		callable->call_output_type = v_extern_ccc.call_output_type;
		callable->call_output_predicted_type = clone_type_map_for_mutation(igc->toc, v_extern_ccc.call_output_type);
		callable->call_output_predicted_type->execution_sequence.clear();
		callable->bundle_id = bundle_id;

		(*p_v_extern_ccc)->underlying_type = std::make_shared<Type>(Type(callable));

		return evaluate_smooth(igc, Type(callable));
	}
	
	if (auto p_v_sizeof = std::get_if<std::shared_ptr<TypeSizeof>>(&type)) {
		const auto& v_sizeof = *p_v_sizeof;

		Smooth target_smooth = evaluate_smooth(igc, *v_sizeof->target);
		
		llvm::Value* target_value = llvm_value(target_smooth);
		llvm::Type* target_llvm_type = target_value->getType();

		// this is the famoius GEP trick to force llvm to give us the size of anything.
		// so even if we, as a compiler, don't know it, our compiled code will happily know it, which is sufficient.
		
		llvm::Value* NULL_pointer = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(target_llvm_type));
		
		llvm::Value* gep = igc->builder->CreateGEP(
			target_llvm_type,
			NULL_pointer,
			llvm::ConstantInt::get(llvm::Type::getInt32Ty(*igc->context), 1) // getting size of "1" element.
		);
		
		llvm::Value* size = igc->builder->CreatePtrToInt(gep, llvm::Type::getInt64Ty(*igc->context));

		auto v_rotten = std::make_shared<TypeRotten>();
		v_rotten->type_str = "u64";

		(*p_v_sizeof)->underlying_type = std::make_shared<Type>(Type(v_rotten));

		return std::make_shared<SmoothInt>(SmoothInt{
			Type(v_rotten),
			size,
		});
	}

	if (auto p_v_take_address = std::get_if<std::shared_ptr<TypeTakeAddress>>(&type)) {
		const auto& v_take_address = *p_v_take_address;

		Smooth target_smooth = evaluate_smooth(igc, *v_take_address->target);

		auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&target_smooth);

		if (!p_v_structval) {
			fprintf(stderr, "At the moment, we can only take the address of a map, and it is treated as a reference.\n");
			exit(1);
		}

		auto target_underlying = get_underlying_type(*v_take_address->target);
		auto p_v_target_map = std::get_if<std::shared_ptr<TypeMap>>(&target_underlying);

		if (!p_v_target_map) {
			fprintf(stderr, "Must target a map.\n");
			exit(1);
		}

		llvm::Value* actual_value = (*p_v_structval)->value;
		
		llvm::Value* alloca_address = igc->builder->CreateAlloca(actual_value->getType(), nullptr);
		
		igc->builder->CreateStore(actual_value, alloca_address);

		auto v_map_reference = std::make_shared<TypeMapReference>();
		v_map_reference->target = *p_v_target_map;
		v_map_reference->is_mutable = v_take_address->is_mutable;

		v_take_address->underlying_type = std::make_shared<Type>(Type(v_map_reference));

		return std::make_shared<SmoothMapReference>(SmoothMapReference{
			Type(v_map_reference),
			alloca_address,
			actual_value->getType(),
		});
	}

	if (std::get_if<std::shared_ptr<TypeNullptr>>(&type)) {
		llvm::Type* opque = llvm::PointerType::get(*igc->context, 0);
		llvm::Value* llNULL = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(opque));
		
		return std::make_shared<SmoothVoidptr>(SmoothVoidptr{
			type,
			llNULL,
		});
	}

	if (std::get_if<std::shared_ptr<TypeVoidptr>>(&type)) {
		llvm::Type* flexi_type = llvm::PointerType::get(*igc->context, 0);
		
		llvm::Value* void_value = smooth_void(igc, type)->value;
		
		return std::make_shared<SmoothVoidVoidptr>(SmoothVoidVoidptr{
			type,
			flexi_type,
			void_value,
		});
	}

	if (std::get_if<std::shared_ptr<TypeVoidMapReference>>(&type)) {
		llvm::Type* flexi_type = llvm::PointerType::get(*igc->context, 0);
		
		llvm::Value* void_value = smooth_void(igc, type)->value;

		return std::make_shared<SmoothVoidMapReference>(SmoothVoidMapReference{
			type,
			flexi_type,
			void_value,
		});
	}

	fprintf(stderr, "Unhandled scenario when handling evaluation\n");
	exit(1);
}
