#include "is_eq_type_rotten.hpp"
#include "t_types.hpp"

bool is_eq_type_rotten(
	const TypeRotten& rotten_a,
	const TypeRotten& rotten_b
) {
	return rotten_a.type_str == rotten_b.type_str;
}
