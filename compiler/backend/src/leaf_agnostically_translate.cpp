#include "leaf_agnostically_translate.hpp"

#include "t_smooth.hpp"
#include "t_types.hpp"
#include "get_underlying_type.hpp"
#include "is_type_singletonish.hpp"
#include "llvm_value.hpp"
#include "structwrap.hpp"
#include "extract_leaf.hpp"
#include "produce_call_func.hpp"
#include "is_structwrappable.hpp"

Smooth leaf_agnostically_translate(std::shared_ptr<IrGenCtx> igc, Smooth smooth, std::shared_ptr<TypeMap> target_map) {
	auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth);

	if (!p_v_structval) {
		return smooth; // maybe we should structwrap here too.
	}

	auto v_structval = *p_v_structval;

	unsigned member_idx = 0;

	std::vector<llvm::Type*> new_member_types;
	std::vector<llvm::Value*> new_member_values;
	std::vector<Smooth> brand_new_smooths;
	std::optional<Smooth> translated_leaf = std::nullopt;

	if (target_map->leaf_type.has_value() && !is_type_singletonish(target_map->leaf_type.value())) {
		Smooth field_smooth = v_structval->field_smooths[member_idx];

		Type leaf_underlying = get_underlying_type(target_map->leaf_type.value());
		auto p_leaf_as_map = std::get_if<std::shared_ptr<TypeMap>>(&leaf_underlying);

		Smooth translated_smooth = [&]() -> Smooth {
			if (!is_structwrappable(field_smooth)) {
				return field_smooth;
			}

			Smooth wrapped = structwrap(igc, field_smooth);

			if (p_leaf_as_map) {
				return leaf_agnostically_translate(igc, wrapped, *p_leaf_as_map);
			}

			return extract_leaf(igc, wrapped);
		}();

		translated_leaf = translated_smooth;

		llvm::Value* translated_value = llvm_value(translated_smooth);
		
		new_member_types.push_back(translated_value->getType());
		new_member_values.push_back(translated_value);
		brand_new_smooths.push_back(translated_smooth);

		member_idx++;
	}

	for (const auto& [sym_name, sym_type] : target_map->sym_inputs) {
		if (is_type_singletonish(*sym_type)) {
			continue;
		}

		Smooth field_smooth = v_structval->field_smooths[member_idx];

		Type sym_underlying = get_underlying_type(*sym_type);
		auto p_sym_as_map = std::get_if<std::shared_ptr<TypeMap>>(&sym_underlying);

		Smooth translated_smooth = [&]() -> Smooth {
			if (!is_structwrappable(field_smooth)) {
				return field_smooth;
			}

			Smooth wrapped = structwrap(igc, field_smooth);

			if (p_sym_as_map) {
				return leaf_agnostically_translate(igc, wrapped, *p_sym_as_map);
			}

			return extract_leaf(igc, wrapped);
		}();

		llvm::Value* translated_value = llvm_value(translated_smooth);
		
		new_member_types.push_back(translated_value->getType());
		new_member_values.push_back(translated_value);
		brand_new_smooths.push_back(translated_smooth);

		member_idx++;
	}

	llvm::StructType* translated_outcome_type = llvm::StructType::get(*igc->context, new_member_types);
	llvm::Value* translated_outcome_value = llvm::UndefValue::get(translated_outcome_type);

	for (size_t i = 0; i < new_member_values.size(); i++) {
		translated_outcome_value = igc->builder->CreateInsertValue(translated_outcome_value, new_member_values[i], (unsigned)i);
	}

	return std::make_shared<SmoothStructval>(SmoothStructval{
		Type(target_map),
		translated_outcome_value,
		translated_leaf.has_value(),
		translated_leaf,
		[igc, target_map]() mutable -> llvm::Function* {
			return produce_call_func(igc, target_map);
		},
		[igc, target_map]() mutable -> llvm::Function* {
			return produce_call_func(igc, target_map, true);
		},
		brand_new_smooths,
	});
}
