#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include "merge_type_enum.hpp"
#include "t_types.hpp"

std::shared_ptr<TypeEnum> merge_type_enum(std::shared_ptr<TypeEnum> enum_a, std::shared_ptr<TypeEnum> enum_b) {
	auto merged = std::make_shared<TypeEnum>();

	for (const auto& sym : enum_a->syms) {
		merged->syms.push_back(sym);
	}

	for (const auto& sym : enum_b->syms) {
		bool already_present = false;

		for (const auto& existing : merged->syms) {
			if (existing == sym) {
				already_present = true;
				break;
			}
		}

		if (!already_present) {
			merged->syms.push_back(sym);
		}
	}

	if (enum_a->hardsym.has_value() && enum_b->hardsym.has_value()) {
		if (enum_a->hardsym.value() != enum_b->hardsym.value()) {
			fprintf(stderr, "no, cannot combine two enum syms simultaneously %s and %s.\n", enum_a->hardsym.value().c_str(), enum_b->hardsym.value().c_str());
			exit(1);
		}

		merged->hardsym = enum_a->hardsym;
		merged->is_instantiated = true;
	} else if (enum_a->hardsym.has_value()) {
		merged->hardsym = enum_a->hardsym;
		merged->is_instantiated = true;
	} else if (enum_b->hardsym.has_value()) {
		merged->hardsym = enum_b->hardsym;
		merged->is_instantiated = true;
	} else if (enum_a->is_instantiated || enum_b->is_instantiated) {
		merged->is_instantiated = true;
	}

	return merged;
}
