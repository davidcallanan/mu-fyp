#include "is_subset_type_enum.hpp"

bool is_subset_type_enum(const TypeEnum& enum_a, const TypeEnum& enum_b) {
	if (enum_a.hardsym.has_value()) {
		bool found = false;

		for (const auto& sym : enum_b.syms) {
			if (sym == enum_a.hardsym.value()) {
				found = true;
				break;
			}
		}

		if (!found) {
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
