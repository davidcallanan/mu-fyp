#include <cstdio>
#include <cstdlib>
#include "merge_underlying_type.hpp"
#include "merge_type_map.hpp"
#include "merge_type_enum.hpp"
#include "get_underlying_type.hpp"
#include "is_eq_type_rotten.hpp"
#include "is_eq_type_pointer.hpp"
#include "t_types.hpp"

Type merge_underlying_type(Type type_a, Type type_b) {
	Type underlying_a = get_underlying_type(type_a);
	Type underlying_b = get_underlying_type(type_b);

	auto p_map_a = std::get_if<std::shared_ptr<TypeMap>>(&underlying_a);
	auto p_map_b = std::get_if<std::shared_ptr<TypeMap>>(&underlying_b);

	if (p_map_a && p_map_b) {
		return merge_type_map(*p_map_a, *p_map_b);
	}

	auto p_enum_a = std::get_if<std::shared_ptr<TypeEnum>>(&underlying_a);
	auto p_enum_b = std::get_if<std::shared_ptr<TypeEnum>>(&underlying_b);

	if (p_enum_a && p_enum_b) {
		return merge_type_enum(*p_enum_a, *p_enum_b);
	}

	auto p_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(&underlying_a);
	auto p_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(&underlying_b);

	if (p_rotten_a && p_rotten_b) {
		const TypeRotten& rotten_a = **p_rotten_a;
		const TypeRotten& rotten_b = **p_rotten_b;

		if (!is_eq_type_rotten(rotten_a, rotten_b)) {
			fprintf(stderr, "Rotten conflict could not be dealth with: %s -vs- %s\n", rotten_a.type_str.c_str(), rotten_b.type_str.c_str());
			exit(1);
		}

		return underlying_a;
	}

	auto p_pointer_a = std::get_if<std::shared_ptr<TypePointer>>(&underlying_a);
	auto p_pointer_b = std::get_if<std::shared_ptr<TypePointer>>(&underlying_b);

	if (p_pointer_a && p_pointer_b) {
		const TypePointer& pointer_a = **p_pointer_a;
		const TypePointer& pointer_b = **p_pointer_b;

		if (!is_eq_type_pointer(pointer_a, pointer_b)) {
			fprintf(stderr, "Pointer conflict, two pointers are not equivalent.\n");
			exit(1);
		}

		return underlying_a;
	}

	fprintf(stderr, "these types can not be merged statically.\n");
	exit(1);
}
