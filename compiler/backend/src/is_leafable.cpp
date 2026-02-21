#include "is_leafable.hpp"

#include "t_types.hpp"

bool is_leafable(const Type& type) {
	return (false
		|| std::get_if<std::shared_ptr<TypePointer>>(&type) != nullptr
		|| std::get_if<std::shared_ptr<TypeRotten>>(&type) != nullptr
	);
}
