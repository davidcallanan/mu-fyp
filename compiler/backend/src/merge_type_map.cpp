#include <cstdio>
#include <cstdlib>
#include "merge_type_map.hpp"
#include "merge_underlying_type.hpp"
#include "t_types.hpp"

std::shared_ptr<TypeMap> merge_type_map(std::shared_ptr<TypeMap> map_a, std::shared_ptr<TypeMap> map_b) {
	if (!map_a->execution_sequence.empty() || !map_b->execution_sequence.empty()) {
		fprintf(stderr, "Instructoins should not be present, expected to be processed already by this point.\n");
		exit(1);
	}
	
	if (map_a->call_input_type != nullptr && map_b->call_input_type != nullptr) {
		fprintf(stderr, "Merge not implemented.\n");
		exit(1);
	}
	
	if (map_a->call_output_type != nullptr && map_b->call_output_type != nullptr) {
		fprintf(stderr, "Merge not implemented.\n");
		exit(1);
	}
	
	if (!map_a->sym_inputs.empty() && !map_b->sym_inputs.empty()) {
		fprintf(stderr, "Merge not implemented!\n");
		exit(1);
	}
	
	auto merged = std::make_shared<TypeMap>(*map_b);

	if (map_a->leaf_type.has_value() && map_b->leaf_type.has_value()) {
		// not really sure if this is the best approach but essentially don't want to lose context (whether we need it later or not).
		Type modern_leaf_situation = merge_underlying_type(map_a->leaf_type.value(), map_b->leaf_type.value());
		auto new_merged_wrapper = std::make_shared<TypeMerged>();
		
		new_merged_wrapper->types.push_back(map_a->leaf_type.value());
		new_merged_wrapper->types.push_back(map_b->leaf_type.value());
		
		new_merged_wrapper->underlying_type = std::make_shared<Type>(modern_leaf_situation);
		
		merged->leaf_type = Type(new_merged_wrapper);
	} else if (map_a->leaf_type.has_value()) {
		fprintf(stderr, "One side has leaf (left side)...");
		fprintf(stderr, "Expect buggy behaviour");
		exit(1);
	} else if (map_b->leaf_type.has_value()) {
		fprintf(stderr, "One side has leaf (right sisde)...");
		fprintf(stderr, "Expect buggy behaviour");
		exit(1);
	}
	
	if (map_a->call_input_type != nullptr) {
		merged->call_input_type = map_a->call_input_type;
		merged->is_this_mutable = map_a->is_this_mutable;
	}
	
	if (map_a->call_output_type != nullptr) {
		merged->call_output_type = map_a->call_output_type;
		merged->call_output_predicted_type = map_a->call_output_predicted_type;
	}
	
	if (!map_a->sym_inputs.empty()) {
		merged->sym_inputs = map_a->sym_inputs;
	}
	
	// todo: consider sym_inputs
	
	return merged;
}
