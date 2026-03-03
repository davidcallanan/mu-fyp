#include "is_mapwrappable.hpp"
#include "t_types_fwd.hpp"

bool is_mapwrappable(const Type& type) {
	return (false
		|| std::holds_alternative<std::shared_ptr<TypeMap>>(type)
		|| std::holds_alternative<std::shared_ptr<TypePointer>>(type)
		|| std::holds_alternative<std::shared_ptr<TypeRotten>>(type)
	);
}
