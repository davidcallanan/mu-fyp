#include <cstdio>
#include <cstdlib>
#include <variant>
#include "is_eq_type_pointer.hpp"
#include "is_eq_type_rotten.hpp"
#include "t_types.hpp"

bool is_eq_type_pointer(
	const TypePointer& pointer_a,
	const TypePointer& pointer_b
) {
	if (pointer_a.target == nullptr || pointer_b.target == nullptr) {
		fprintf(stderr, "Pointer to nothing?\n");
		exit(1);
	}
	
	const Type& target_a = *pointer_a.target;
	const Type& target_b = *pointer_b.target;
	
	auto p_v_pointer_a = std::get_if<std::shared_ptr<TypePointer>>(&target_a);
	auto p_v_pointer_b = std::get_if<std::shared_ptr<TypePointer>>(&target_b);
	
	if (p_v_pointer_a && p_v_pointer_b) {
		return is_eq_type_pointer(**p_v_pointer_a, **p_v_pointer_b);
	}
	
	auto p_v_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(&target_a);
	auto p_v_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(&target_b);
	
	if (p_v_rotten_a && p_v_rotten_b) {
		return is_eq_type_rotten(**p_v_rotten_a, **p_v_rotten_b);
	}
	
	fprintf(stderr, "For now, pointers to non-rotten types are not implemented appropriately.\n");
	exit(1);
}
