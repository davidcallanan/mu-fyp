#include "is_subset_type_map.hpp"
#include "is_subset_type.hpp"

bool is_subset_type_map(const TypeMap& map_a, const TypeMap& map_b) {
	bool a_has_leaf = map_a.leaf_type != nullptr || map_a.leaf_hardval != nullptr;
	bool a_needs_leaf = map_b.leaf_type != nullptr || map_b.leaf_hardval != nullptr;

	if (a_needs_leaf && !a_has_leaf) {
		return false;
	}

	for (const auto& [sym_name_b, sym_type_b] : map_b.sym_inputs) {
		auto it = map_a.sym_inputs.find(sym_name_b);

		if (it == map_a.sym_inputs.end()) {
			return false;
		}

		if (!is_subset_type(*it->second, *sym_type_b)) {
			return false;
		}
	}

	return true;
}
