#include "leaf_agnostically_translate.hpp"

#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_ctx.hpp"
#include "t_bundles.hpp"
#include "get_underlying_type.hpp"
#include "is_type_singletonish.hpp"
#include "llvm_value.hpp"
#include "structwrap.hpp"
#include "extract_leaf.hpp"
#include "produce_call_func.hpp"
#include "is_structwrappable.hpp"
#include "force_identical_layout.hpp"
#include "structval_field_idx.hpp"
#include "preinstantiated_smooths.hpp"

Smooth leaf_agnostically_translate(std::shared_ptr<IrGenCtx> igc, Smooth smooth, std::shared_ptr<TypeMap> target_map, bool use_flexi_mode) {
	auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth);

	if (!p_v_structval) {
		return smooth; // maybe we should structwrap here too.
	}

	auto v_structval = *p_v_structval;

	// std::vector<llvm::Type*> new_member_types;
	std::vector<llvm::Value*> new_member_values;
	std::vector<Smooth> brand_new_smooths;
	std::optional<Smooth> translated_leaf = std::nullopt;

	if (target_map->leaf_type.has_value() && !is_type_singletonish(target_map->leaf_type.value())) {
		if (v_structval->field_smooths.empty()) {
			fprintf(stderr, "Map reports itself as having leaf, but no leaf existed (field_smooths empty!).\n");
			exit(1);
		}
		
		Smooth field_smooth = v_structval->field_smooths[0];

		Type leaf_underlying = get_underlying_type(target_map->leaf_type.value());
		auto p_leaf_as_map = std::get_if<std::shared_ptr<TypeMap>>(&leaf_underlying);

		Smooth translated_smooth = [&]() -> Smooth {
			if (!is_structwrappable(field_smooth)) {
				return field_smooth;
			}

			Smooth wrapped = structwrap(igc, field_smooth);

			if (p_leaf_as_map) {
				return leaf_agnostically_translate(igc, wrapped, *p_leaf_as_map, use_flexi_mode);
			}

			return extract_leaf(igc, wrapped);
		}();

		translated_leaf = translated_smooth;

		llvm::Value* translated_value = llvm_value(translated_smooth);
		
		// new_member_types.push_back(translated_value->getType());
		new_member_values.push_back(translated_value);
		brand_new_smooths.push_back(translated_smooth);
	}

	for (const auto& [sym_name, sym_type] : target_map->sym_inputs) {
		if (is_type_singletonish(*sym_type)) {
			continue;
		}

		auto source_idx = structval_field_idx(v_structval, sym_name);

		Smooth field_smooth = [&]() -> Smooth {
			if (source_idx.has_value()) {
				return v_structval->field_smooths[source_idx.value()];
			}

			fprintf(stderr, "Implicitely populating missing field with {}\n");
			
			Type intended_type = get_underlying_type(*sym_type);
			
			auto intended_as_map = std::get_if<std::shared_ptr<TypeMap>>(&intended_type);
			
			if (!intended_as_map) {
				fprintf(stderr, "Failed to implicitely populate missing field, because it wasn't a map. %s\n", sym_name.c_str());
				fprintf(stderr, "Did you make a map, and force upon it a type, containing syms that you forgot the populate?\n");
				fprintf(stderr, "Did you forget to populate '%s'?\n", sym_name.c_str());
				exit(1);
			}

			return leaf_agnostically_translate(igc, smooth_map_empty(igc), *intended_as_map, use_flexi_mode);
		}();

		Type sym_underlying = get_underlying_type(*sym_type);
		auto p_sym_as_map = std::get_if<std::shared_ptr<TypeMap>>(&sym_underlying);

		Smooth translated_smooth = [&]() -> Smooth {
			if (!is_structwrappable(field_smooth)) {
				return field_smooth;
			}

			Smooth wrapped = structwrap(igc, field_smooth);

			if (p_sym_as_map) {
				return leaf_agnostically_translate(igc, wrapped, *p_sym_as_map, use_flexi_mode);
			}

			return extract_leaf(igc, wrapped);
		}();

		llvm::Value* translated_value = llvm_value(translated_smooth);
		
		// new_member_types.push_back(translated_value->getType());
		new_member_values.push_back(translated_value);
		brand_new_smooths.push_back(translated_smooth);
	}

	if (!target_map->bundle_id.has_value()) {
		fprintf(stderr, "Bundle gone to stray.\n");
		exit(1);
	}

	Bundle* bundle = igc->toc->bundle_registry->get(target_map->bundle_id.value());

	if (!bundle) {
		fprintf(stderr, "Bundle missing in action.\n");
		exit(1);
	}

	auto p_bundle_map = std::get_if<std::shared_ptr<BundleMap>>(bundle);

	if (!p_bundle_map) {
		fprintf(stderr, "BundleMap turned up missing.\n");
		exit(1);
	}

	llvm::StructType* translated_outcome_type = use_flexi_mode
		? (*p_bundle_map)->opaque_flexi_struct_type
		: (*p_bundle_map)->opaque_struct_type;

	if (!translated_outcome_type || translated_outcome_type->isOpaque()) {
		fprintf(stderr, "Opaque struct's body woke up dead.\n");
		exit(1);
	}
	
	if (translated_outcome_type->isOpaque()) {
		fprintf(stderr, "struct still opaque %s.\n", use_flexi_mode ? "opaque_flexi_struct_type" : "opaque_struct_type");
		exit(1);
	}

	llvm::Value* translated_outcome_value = llvm::UndefValue::get(translated_outcome_type);

	for (size_t i = 0; i < new_member_values.size(); i++) {
		llvm::Type* expected = translated_outcome_type->getElementType(i);
		llvm::Value* desired = force_identical_layout(igc, new_member_values[i], expected);
		
		if (desired->getType() != expected) {
			fprintf(stderr, "leaf_agnostically_translate struggled with %zu\n", i);
			fprintf(stderr, "WANTED:");
			expected->print(llvm::errs());
			fprintf(stderr, "\nGHOT:");
			desired->getType()->print(llvm::errs());
			fprintf(stderr, "\n- target has leaf? %s\n", target_map->leaf_type.has_value() ? "true" : "false");
			fprintf(stderr, "- num sym inputs: %zu\n", target_map->sym_inputs.size());
			fprintf(stderr, "- use_flexi_mode: %s\n", use_flexi_mode ? "true" : "false");
			fprintf(stderr, "Did you run the smooth through happy_smooth before calling leaf_agnostically_translate?\n");
			exit(1);
		}
		
		translated_outcome_value = igc->builder->CreateInsertValue(translated_outcome_value, desired, (unsigned) i);
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
