#include "is_subset_type.hpp"
#include "is_subset_type_map.hpp"
#include "t_types.hpp"

bool is_subset_type(const Type& type_a, const Type& type_b) {
	if (type_a.index() != type_b.index()) {
		return false;	
	}
	
	// todo: do we have to promote a pointer to a map if other is a map?
	
	if (auto p_v_map_a = std::get_if<std::shared_ptr<TypeMap>>(&type_a)) {
		auto p_v_map_b = std::get_if<std::shared_ptr<TypeMap>>(&type_b);
		return is_subset_type_map(**p_v_map_a, **p_v_map_b);
	}

	if (auto p_v_ptr_a = std::get_if<std::shared_ptr<TypePointer>>(&type_a)) {
		auto p_v_ptr_b = std::get_if<std::shared_ptr<TypePointer>>(&type_b);
		return is_subset_type(*(*p_v_ptr_a)->target, *(*p_v_ptr_b)->target);
	}

	if (auto p_v_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(&type_a)) {
		auto p_v_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(&type_b);
		return (*p_v_rotten_a)->type_str == (*p_v_rotten_b)->type_str; // language will require explicit casting for now.
	}

	// for now we pretend everything else is incompatible.
	return false;
}
