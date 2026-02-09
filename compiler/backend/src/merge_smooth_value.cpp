#include <cstdio>
#include <cstdlib>
#include <variant>
#include "llvm/IR/DerivedTypes.h"
#include "merge_smooth_value.hpp"
#include "is_eq_type_pointer.hpp"
#include "is_eq_type_rotten.hpp"
#include "t_types.hpp"

SmoothValue merge_smooth_value(
	IrGenCtx& igc,
	const SmoothValue& smooth_a,
	const SmoothValue& smooth_b
) {
	auto p_v_map_a = std::get_if<std::shared_ptr<TypeMap>>(&smooth_a.type);
	auto p_v_map_b = std::get_if<std::shared_ptr<TypeMap>>(&smooth_b.type);
	
	if (!p_v_map_a || !p_v_map_b) {
		fprintf(stderr, "only makes sense to merge two maps for now, really i don't think there should ever be non-map as a smooth_value.\n");
		exit(1);
	}
	
	const TypeMap& map_a = **p_v_map_a;
	const TypeMap& map_b = **p_v_map_b;
	
	if (!map_a.execution_sequence.empty() || !map_b.execution_sequence.empty()) {
		fprintf(stderr, "Instructoins should not be present, expected to be processed already by this point.\n");
		exit(1);
	}
	
	if (map_a.call_input_type != nullptr && map_b.call_input_type != nullptr) {
		fprintf(stderr, "Merge not implemented.\n");
		exit(1);
	}
	
	if (map_a.call_output_type != nullptr && map_b.call_output_type != nullptr) {
		fprintf(stderr, "Merge not implemented.\n");
		exit(1);
	}
	
	if (!map_a.sym_inputs.empty() && !map_b.sym_inputs.empty()) {
		fprintf(stderr, "Merge not implemented!\n");
		exit(1);
	}
	
	if (map_a.leaf_type != nullptr && map_b.leaf_type != nullptr) {
		auto p_v_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(map_a.leaf_type.get());
		auto p_v_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(map_b.leaf_type.get());
		
		if (p_v_rotten_a && p_v_rotten_b) {
			const TypeRotten& rotten_a = **p_v_rotten_a;
			const TypeRotten& rotten_b = **p_v_rotten_b;
			
			if (!is_eq_type_rotten(rotten_a, rotten_b)) {
				fprintf(stderr, "Rotten conflict could not be dealth with: %s -vs- %s\n", rotten_a.type_str.c_str(), rotten_b.type_str.c_str());
				exit(1);
			}
		}
		
		auto p_v_pointer_a = std::get_if<std::shared_ptr<TypePointer>>(map_a.leaf_type.get());
		auto p_v_pointer_b = std::get_if<std::shared_ptr<TypePointer>>(map_b.leaf_type.get());
		
		if (p_v_pointer_a && p_v_pointer_b) {
			const TypePointer& pointer_a = **p_v_pointer_a;
			const TypePointer& pointer_b = **p_v_pointer_b;
			
			if (!is_eq_type_pointer(pointer_a, pointer_b)) {
				fprintf(stderr, "Pointer conflict, two pointers are not equivalent.\n");
				exit(1);
			}
		}
		
		fprintf(stderr, "Cannot merge conflicting leaf types or unhandled leaf types.\n");
	}
	
	auto p_merged = std::make_shared<TypeMap>(map_b);
	
	if (map_a.leaf_type != nullptr) {
		p_merged->leaf_type = map_a.leaf_type;
	}
	
	if (map_a.call_input_type != nullptr) {
		p_merged->call_input_type = map_a.call_input_type;
	}
	
	if (map_a.call_output_type != nullptr) {
		p_merged->call_output_type = map_a.call_output_type;
	}
	
	if (!map_a.sym_inputs.empty()) {
		p_merged->sym_inputs = map_a.sym_inputs;
	}
	
	return SmoothValue{
		smooth_b.struct_value,
		p_merged,
		smooth_b.has_leaf,
	};
}
