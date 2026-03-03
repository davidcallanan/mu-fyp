#include "extract_map_leaf.hpp"
#include "get_underlying_type.hpp"
#include "is_mapwrappable.hpp"
#include "t_types.hpp"

Type extract_map_leaf(const Type& type, bool be_permissive) {
	Type underlying = get_underlying_type(type);
	auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying);

	if (p_v_map) {
		if (!(*p_v_map)->leaf_type.has_value()) {
			fprintf(stderr, "there's no leaf type known for this map.\n");
			exit(1);
		}

		return (*p_v_map)->leaf_type.value();
	}

	if (!be_permissive) {
		fprintf(stderr, "be_permissive was not selected, but the passed type is not capable of holding a leaf.\n");
		exit(1);
	}

	if (!is_mapwrappable(type)) {
		fprintf(stderr, "While be_permissive was selected, the provided type could not be converted to something capable of holding a leaf.\n");
		exit(1);
	}

	return type;
}
