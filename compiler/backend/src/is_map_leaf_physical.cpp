#include "is_map_leaf_physical.hpp"

#include "t_types.hpp"
#include "is_type_singletonish.hpp"

bool is_map_leaf_physical(std::shared_ptr<TypeMap> map) {
	return (true // think this is right
		&& map->leaf_type.has_value()
		&& !is_type_singletonish(map->leaf_type.value())
	);
}
