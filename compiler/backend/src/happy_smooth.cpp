#include "happy_smooth.hpp"

#define TMP_ALLOW_IMPLICIT_BIT_CHOPPING 1

#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_ctx.hpp"
#include "t_bundles.hpp"
#include "get_underlying_type.hpp"
#include "rotten_int_info.hpp"
#include "rotten_float_info.hpp"
#include "is_type_singletonish.hpp"
#include "llvm_value.hpp"
#include "force_identical_layout.hpp"
#include "better_leaf_agnostically_translate.hpp"
#include "structval_field_idx.hpp"
#include "preinstantiated_smooths.hpp"
#include "leaf_agnostically_translate.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

Smooth happy_smooth(std::shared_ptr<IrGenCtx> igc, Smooth smooth, const Type& type, bool use_flexi_mode) {
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
			const char* mytypeid = std::visit([](auto&& v) { return typeid(*v).name(); }, smooth);
			fprintf(stderr, "Actual smooth wass %s\n", mytypeid);
			exit(1);
		}
		
		auto v_structval = *p_v_structval;

		if (v_structval->field_smooths.empty()) {
			// just gonna assume empty maps will match the passed in type.
			return smooth;
		}

		// std::vector<llvm::Type*> new_member_types;
		std::vector<llvm::Value*> new_member_values;
		std::vector<Smooth> converted_field_smooths;
		std::optional<Smooth> brand_new_leaf = std::nullopt;

		if (true
			&& v_map->leaf_type.has_value()
			&& !is_type_singletonish(v_map->leaf_type.value())
		) {
			Smooth field_smooth = v_structval->field_smooths[0];
			Smooth field_happy = happy_smooth(igc, field_smooth, v_map->leaf_type.value(), use_flexi_mode);
			Smooth field_translated = better_leaf_agnostically_translate(igc, field_happy, v_map->leaf_type.value(), use_flexi_mode);
			llvm::Value* field_happy_value = llvm_value(field_translated);

			// new_member_types.push_back(field_happy_value->getType());
			new_member_values.push_back(field_happy_value);
			converted_field_smooths.push_back(field_translated);
			
			brand_new_leaf = field_translated;
		}

		for (const auto& [sym_name, sym_type] : v_map->sym_inputs) {
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
				
				if ((*intended_as_map)->leaf_type.has_value()) {
					fprintf(stderr, "Cannot implicitely poopulate lief field.\n", sym_name.c_str());
					fprintf(stderr, "You have tried to instantiate a map without a leaf, and use it somewhere expecting a leaf\n");
					fprintf(stderr, "Other syms:");
					
					for (const auto& [other, _] : (*intended_as_map)->sym_inputs) {
						fprintf(stderr, "-- %s\n", other.c_str());
					}
					
					exit(1);
				}

				return leaf_agnostically_translate(igc, smooth_map_empty(igc), *intended_as_map, use_flexi_mode);
			}();

			Smooth field_happy = happy_smooth(igc, field_smooth, *sym_type, use_flexi_mode);
			Smooth field_translated = better_leaf_agnostically_translate(igc, field_happy, *sym_type, use_flexi_mode);
			llvm::Value* field_happy_value = llvm_value(field_translated);

			// new_member_types.push_back(field_happy_value->getType());
			new_member_values.push_back(field_happy_value);
			converted_field_smooths.push_back(field_translated);
		}

		if (!v_map->bundle_id.has_value()) {
			fprintf(stderr, "No bundle.\n");
			exit(1);
		}

		Bundle* bundle = igc->toc->bundle_registry->get(v_map->bundle_id.value());

		if (!bundle) {
			fprintf(stderr, "No bundle\n");
			exit(1);
		}

		auto p_bundle_map = std::get_if<std::shared_ptr<BundleMap>>(bundle);

		if (!p_bundle_map) {
			fprintf(stderr, "No map-based bundle\n");
			exit(1);
		}

		llvm::StructType* fancy_type = use_flexi_mode
			? (*p_bundle_map)->opaque_flexi_struct_type
			: (*p_bundle_map)->opaque_struct_type;

		if (!fancy_type || fancy_type->isOpaque()) {
			fprintf(stderr, "type still opquaeuea.\n");
			exit(1);
		}

		llvm::Value* fancy_value = llvm::UndefValue::get(fancy_type);

		for (size_t i = 0; i < new_member_values.size(); i++) {
			llvm::Type* what_we_really_want = fancy_type->getElementType(i);
			llvm::Value* desired_value = force_identical_layout(igc, new_member_values[i], what_we_really_want);
			
			if (desired_value->getType() != what_we_really_want) {
#if TMP_ALLOW_IMPLICIT_BIT_CHOPPING
				if (desired_value->getType()->isIntegerTy() && what_we_really_want->isIntegerTy()) {
					desired_value = igc->builder->CreateTrunc(desired_value, what_we_really_want);
				} else {
#endif
				fprintf(stderr, "happy_smooth struggled with %zu\n", i);
				fprintf(stderr, "WANTED:");
				what_we_really_want->print(llvm::errs());
				fprintf(stderr, "\nGHOT:");
				desired_value->getType()->print(llvm::errs());
				fprintf(stderr, "\n- target has leaf? %s\n", v_map->leaf_type.has_value() ? "true" : "false");
				fprintf(stderr, "- num sym inputs: %zu\n", v_map->sym_inputs.size());
				fprintf(stderr, "- use_flexi_mode: %s\n", use_flexi_mode ? "true" : "false");
				exit(1);
#if TMP_ALLOW_IMPLICIT_BIT_CHOPPING
				}
#endif
			}
		
			fancy_value = igc->builder->CreateInsertValue(fancy_value, desired_value, (unsigned) i);
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
