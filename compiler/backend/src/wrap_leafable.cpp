#include "wrap_leafable.hpp"
#include "t_types.hpp"

TypeMap wrap_leafable(const Type& leafable) {
	TypeMap result;
	result.leaf_type = leafable;
	result.leaf_hardval = std::nullopt;
	return result;
}
