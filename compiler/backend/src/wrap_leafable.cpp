#include "wrap_leafable.hpp"
#include "t_types.hpp"

TypeMap wrap_leafable(const Type& leafable) {
	TypeMap result;
	result.leaf_type = std::make_shared<Type>(leafable);
	result.leaf_hardval = nullptr;
	return result;
}
