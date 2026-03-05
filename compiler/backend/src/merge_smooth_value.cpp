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

std::shared_ptr<SmoothStructval> merge_smooth_structval(
	IrGenCtx& igc,
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
		smooth_leaf = merge_smooth(igc, structval_a->leaf.value(), structval_b->leaf.value());

		llvm::StructType* struct_type_orig = llvm::cast<llvm::StructType>(structval_b->value->getType());
		std::vector<llvm::Type*> field_types(struct_type_orig->element_begin(), struct_type_orig->element_end());
		
		field_types[0] = llvm_value(smooth_leaf.value())->getType();

		llvm::StructType* struct_type_new = llvm::StructType::get(igc.context, field_types);
		llvm::Value* replaced_value = llvm::UndefValue::get(struct_type_new);

		replaced_value = igc.builder.CreateInsertValue(replaced_value, llvm_value(smooth_leaf.value()), 0);

		// currently only taking sym inputs from the b value, not properly merging this stuff yet.
		
		for (uint32_t i = 1; i < struct_type_orig->getNumElements(); i++) {
			llvm::Value* element = igc.builder.CreateExtractValue(structval_b->value, i);
			replaced_value = igc.builder.CreateInsertValue(replaced_value, element, i);
		}

		value_merged = replaced_value;
	}

	return std::make_shared<SmoothStructval>(SmoothStructval{
		type_merged,
		value_merged,
		structval_b->has_leaf,
		smooth_leaf,
	});
}
