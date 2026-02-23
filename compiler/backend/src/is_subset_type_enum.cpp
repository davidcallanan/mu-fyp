#include "is_subset_type_enum.hpp"

bool is_subset_type_enum(const TypeEnum& enum_a, const TypeEnum& enum_b) {
	if (enum_b.hardsym.has_value()) {
		if (!enum_a.hardsym.has_value() || enum_a.hardsym.value() != enum_b.hardsym.value()) {
			return false;
		}
	}

	for (const auto& sym : enum_a.syms) {
		bool found = false;
		
		for (const auto& b_sym : enum_b.syms) {
			if (sym == b_sym) {
				found = true;
				break;
			}
		}
		
		if (!found) {
			return false;
		}
	}

	return true;
}
