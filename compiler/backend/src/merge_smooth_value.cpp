#include <cstdio>
#include <cstdlib>
#include <variant>
#include "merge_smooth_value.hpp"
#include "merge_smooth.hpp"
#include "merge_type_map.hpp"
#include "llvm_value.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "extract_leaf.hpp"
#include "smooth_type.hpp"
#include "get_underlying_type.hpp"
#include "is_type_singletonish.hpp"
#include "evaluate_singletonish.hpp"
#include "happy_smooth.hpp"

std::shared_ptr<SmoothStructval> merge_smooth_structval(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothStructval> structval_a,
	std::shared_ptr<SmoothStructval> structval_b
) {
	auto p_v_map_a = std::get_if<std::shared_ptr<TypeMap>>(&structval_a->type);
	auto p_v_map_b = std::get_if<std::shared_ptr<TypeMap>>(&structval_b->type);
	
	if (!p_v_map_a || !p_v_map_b) {
		fprintf(stderr, "only makes sense to merge two maps for now, really i don't think there should ever be non-map as a smooth_value.\n");
		exit(1);
	}

	std::shared_ptr<TypeMap> type_merged = merge_type_map(*p_v_map_a, *p_v_map_b);

	llvm::Value* value_merged = structval_b->value;
	std::optional<Smooth> smooth_leaf = std::nullopt;

	if (structval_a->has_leaf && structval_b->has_leaf) {
		if (is_type_singletonish(type_merged->leaf_type.value())) {
			smooth_leaf = evaluate_singletonish(igc, type_merged->leaf_type.value());
		} else {
			smooth_leaf = happy_smooth(
				igc,
				merge_smooth(igc, extract_leaf(igc, Smooth(structval_a)), extract_leaf(igc, Smooth(structval_b))),
				type_merged->leaf_type.value()
			);
		}

		auto underlying = get_underlying_type(smooth_type(smooth_leaf.value()));
		bool is_leaf_physical = !is_type_singletonish(underlying);

		llvm::StructType* struct_type_orig = llvm::cast<llvm::StructType>(structval_b->value->getType());
		std::vector<llvm::Type*> field_types(struct_type_orig->element_begin(), struct_type_orig->element_end());

		if (is_leaf_physical) {
			field_types[0] = llvm_value(smooth_leaf.value())->getType();
		}

		llvm::StructType* struct_type_new = llvm::StructType::get(*igc->context, field_types);
		llvm::Value* replaced_value = llvm::UndefValue::get(struct_type_new);

		if (is_leaf_physical) {
			replaced_value = igc->builder->CreateInsertValue(replaced_value, llvm_value(smooth_leaf.value()), 0);
		}

		// currently only taking sym inputs from the b value, not properly merging this stuff yet.

		for (uint32_t i = is_leaf_physical ? 1 : 0; i < struct_type_orig->getNumElements(); i++) {
			llvm::Value* element = igc->builder->CreateExtractValue(structval_b->value, i);
			replaced_value = igc->builder->CreateInsertValue(replaced_value, element, i);
		}

		value_merged = replaced_value;
	}

	return std::make_shared<SmoothStructval>(SmoothStructval{
		type_merged,
		value_merged,
		structval_b->has_leaf,
		smooth_leaf,
		{}, // this is problematic.
		{}, // todo
		{}, // todo
	});
}
