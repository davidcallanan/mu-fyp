#include "happy_smooth.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "get_underlying_type.hpp"
#include "rotten_int_info.hpp"
#include "rotten_float_info.hpp"
#include "is_type_singletonish.hpp"
#include "llvm_value.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

Smooth happy_smooth(std::shared_ptr<IrGenCtx> igc, Smooth smooth, const Type& type) {
	Type underlying = get_underlying_type(type);

	if (auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&underlying)) {
		if (auto p_v_int = std::get_if<std::shared_ptr<SmoothInt>>(&smooth)) {
			if (!(*p_v_int)->value) {
				return smooth;
			}

			if (auto info = rotten_int_info(*p_v_rotten)) {
				uint32_t actual_bits = (*p_v_int)->value->getType()->getIntegerBitWidth();

				if (actual_bits < info->bits) {
					llvm::Value* extended = igc->builder->CreateZExt(
						(*p_v_int)->value,
						llvm::IntegerType::get(*igc->context, info->bits)
					);
					
	
					return std::make_shared<SmoothInt>(SmoothInt{ (*p_v_int)->type, extended });
				}
			}

			return smooth;
		}

		if (auto p_v_float = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth)) {
			if (!(*p_v_float)->value) {
				return smooth;
			}

			if (auto info = rotten_float_info(*p_v_rotten)) {
				llvm::Type* target_type = nullptr;

				if (info->bits == 16) target_type = llvm::Type::getHalfTy(*igc->context);
				else if (info->bits == 32) target_type = llvm::Type::getFloatTy(*igc->context);
				else if (info->bits == 64) target_type = llvm::Type::getDoubleTy(*igc->context);
				else if (info->bits == 128) target_type = llvm::Type::getFP128Ty(*igc->context);

				if (target_type && (*p_v_float)->value->getType() != target_type) {
					llvm::Value* extended = igc->builder->CreateFPExt(
						(*p_v_float)->value,
						target_type
					);
					
					return std::make_shared<SmoothFloat>(SmoothFloat{ (*p_v_float)->type, extended });
				}
			}

			return smooth;
		}

		return smooth;
	} else if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying)) {
		auto v_map = *p_v_map;
		
		auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth);

		if (!p_v_structval) {
			fprintf(stderr, "Glitch.");
			exit(1);
		}
		
		auto v_structval = *p_v_structval;

		if (v_structval->field_smooths.empty()) {
			// just gonna assume empty maps will match the passed in type.
			return smooth;
		}

		unsigned member_idx = 0;
		
		std::vector<llvm::Type*> new_member_types;
		std::vector<llvm::Value*> new_member_values;
		std::vector<Smooth> converted_field_smooths;
		std::optional<Smooth> brand_new_leaf = std::nullopt;

		if (true
			&& v_map->leaf_type.has_value()
			&& !is_type_singletonish(v_map->leaf_type.value())
		) {
			Smooth field_smooth = v_structval->field_smooths[member_idx];
			Smooth field_happy = happy_smooth(igc, field_smooth, v_map->leaf_type.value());
			llvm::Value* field_happy_value = llvm_value(field_happy);

			new_member_types.push_back(field_happy_value->getType());
			new_member_values.push_back(field_happy_value);
			converted_field_smooths.push_back(field_happy);
			
			brand_new_leaf = field_happy;
			
			member_idx++;
		}

		for (const auto& [sym_name, sym_type] : v_map->sym_inputs) {
			if (is_type_singletonish(*sym_type)) {
				continue;
			}

			Smooth field_smooth = v_structval->field_smooths[member_idx];
			Smooth field_happy = happy_smooth(igc, field_smooth, *sym_type);
			llvm::Value* field_happy_value = llvm_value(field_happy);
			
			new_member_types.push_back(field_happy_value->getType());
			new_member_values.push_back(field_happy_value);
			converted_field_smooths.push_back(field_happy);
			
			member_idx++;
		}

		llvm::StructType* fancy_type = llvm::StructType::get(*igc->context, new_member_types);
		llvm::Value* fancy_value = llvm::UndefValue::get(fancy_type);

		for (size_t i = 0; i < new_member_values.size(); i++) {
			fancy_value = igc->builder->CreateInsertValue(fancy_value, new_member_values[i], (unsigned)i);
		}

		return std::make_shared<SmoothStructval>(SmoothStructval{
			underlying, // i used to not like reducing to underlying, but there is no wrapper type in this context, unless we had a TypeHappy
			fancy_value,
			brand_new_leaf.has_value(),
			brand_new_leaf,
			v_structval->call_func,
			v_structval->call_func_alwaysinline,
			converted_field_smooths,
		});
	}

	return smooth;
}
