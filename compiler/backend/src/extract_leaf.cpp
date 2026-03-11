#include "extract_leaf.hpp"
#include "get_underlying_type.hpp"
#include "is_structwrappable.hpp"
#include "rotten_float_info.hpp"
#include "structwrap.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"

Smooth extract_leaf(std::shared_ptr<IrGenCtx> igc, Smooth smooth, bool be_permissive) {
	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		const auto& v_structval = *p_v_structval;

		if (!v_structval->has_leaf) {
			fprintf(stderr, "Not good circumstances - no leaf.\n");
			fprintf(stderr, "Actually what we got is: ");
			v_structval->value->print(llvm::errs());
				
			fprintf(stderr, "Bad programmer - check for .has_leaf first!\n");
			exit(1);
		}

		if (v_structval->leaf.has_value()) {
			return v_structval->leaf.value();
		}
		
		// legacy system below in case something goes wrong - can possibly delete this code once i've confirmed the entire codebase has migrated to the new system.

		Type underlying = get_underlying_type(v_structval->type);
		auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying);

		if (!p_v_map) {
			fprintf(stderr, "structval must interpret itself as a map after evaluation.\n");
			exit(1);
		}

		const auto& o_leaf_type = (*p_v_map)->leaf_type;
		const auto& o_leaf_hardval = (*p_v_map)->leaf_hardval;

		if (!o_leaf_type.has_value() && !o_leaf_hardval.has_value()) {
			fprintf(stderr, "invariant violation.\n");
			exit(1);
		}

		llvm::Value* value = igc->builder->CreateExtractValue(v_structval->value, 0);

		if (!o_leaf_type.has_value()) {
			auto v_rotten = std::make_shared<TypeRotten>();

			if (value->getType()->isFloatingPointTy()) {
				v_rotten->type_str = "f" + std::to_string(value->getType()->getPrimitiveSizeInBits());
				
				return std::make_shared<SmoothFloat>(SmoothFloat{
					Type(v_rotten),
					value,
				});
			}

			v_rotten->type_str = "i" + std::to_string(value->getType()->getIntegerBitWidth());
			
			return std::make_shared<SmoothInt>(SmoothInt{
				Type(v_rotten),
				value,
			});
		}

		const Type leaf_type = o_leaf_type.value();
		Type leaf_underlying = get_underlying_type(leaf_type);

		if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&leaf_underlying)) {
			return std::make_shared<SmoothEnum>(SmoothEnum{
				leaf_type,
				value,
			});
		}

		if (std::get_if<std::shared_ptr<TypePointer>>(&leaf_underlying)) {
			return std::make_shared<SmoothPointer>(SmoothPointer{
				leaf_type,
				value,
			});
		}

		if (auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&leaf_underlying)) {
			const std::string& type_str = (*p_v_rotten)->type_str;
			
			const bool is_float = (false
				|| type_str == "f16"
				|| type_str == "f32"
				|| type_str == "f64"
				|| type_str == "f128"
			);
			
			if (is_float) {
				return std::make_shared<SmoothFloat>(SmoothFloat{
					leaf_type,
					value,
				});
			}
			
			return std::make_shared<SmoothInt>(SmoothInt{ 
				leaf_type,
				value,
			});
		}

		fprintf(stderr, "Leaf was something we don't know how to deal with\n");
		exit(1);
	}

	if (!be_permissive) {
		fprintf(stderr, "smooth was not some scructval. either pass be_permissive (to auto-wrap before extraction) or ensure exact structval is passed.\n");
		exit(1);
	}

	if (!is_structwrappable(smooth)) {
		fprintf(stderr, "smooth was not structwrawppable. although be_permissive flag was set, we could not auto-wrap such an exoteic object.\n");
		exit(1);
	}

	return extract_leaf(igc, structwrap(igc, smooth));
}
