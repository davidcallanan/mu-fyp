#include "is_subset_type.hpp"
#include "is_subset_type_map.hpp"
#include "is_subset_type_enum.hpp"
#include "is_leafable.hpp"
#include "wrap_leafable.hpp"
#include "get_underlying_type.hpp"
#include "t_types.hpp"

bool is_subset_type(const Type& type_a, const Type& type_b) { // todo: i notice this function doesn't even handle underlying types.
	const Type a = get_underlying_type(type_a);
	const Type b = get_underlying_type(type_b);

	if (std::get_if<std::shared_ptr<TypeMap>>(&a) && is_leafable(b)) {
		Type wrapped = std::make_shared<TypeMap>(wrap_leafable(b));
		return is_subset_type(a, wrapped);
	}
	
	if (std::get_if<std::shared_ptr<TypeMap>>(&b) && is_leafable(a)) {
		Type wrapped = std::make_shared<TypeMap>(wrap_leafable(a));
		return is_subset_type(wrapped, b);
	}

	if (a.index() != b.index()) {
		return false;	
	}
	
	if (auto p_v_map_a = std::get_if<std::shared_ptr<TypeMap>>(&a)) {
		auto p_v_map_b = std::get_if<std::shared_ptr<TypeMap>>(&b);
		return is_subset_type_map(**p_v_map_a, **p_v_map_b);
	}

	if (auto p_v_ptr_a = std::get_if<std::shared_ptr<TypePointer>>(&a)) {
		auto p_v_ptr_b = std::get_if<std::shared_ptr<TypePointer>>(&b);
		return is_subset_type(*(*p_v_ptr_a)->target, *(*p_v_ptr_b)->target);
	}

	if (auto p_v_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(&a)) {
		auto p_v_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(&b);
		return (*p_v_rotten_a)->type_str == (*p_v_rotten_b)->type_str; // language will require explicit casting for now.
	}

	if (auto p_v_enum_a = std::get_if<std::shared_ptr<TypeEnum>>(&a)) {
		auto p_v_enum_b = std::get_if<std::shared_ptr<TypeEnum>>(&b);
		return is_subset_type_enum(**p_v_enum_a, **p_v_enum_b);
	}
	
	// explicit casting required for pointers, void pointers, null pointers, references!

	// for now we pretend everything else is incompatible.
	return false;
}
