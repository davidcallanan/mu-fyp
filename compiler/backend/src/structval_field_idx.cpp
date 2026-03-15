#include "structval_field_idx.hpp"

#include <cstdio>
#include <cstdlib>
#include "t_types.hpp"
#include "t_smooth.hpp"
#include "map_sym_idx.hpp"

std::optional<size_t> structval_field_idx(std::shared_ptr<SmoothStructval> structval, const std::string& sym_name) {
	auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&structval->type);

	if (!p_v_map) {
		fprintf(stderr, "Structval was not determined by a map - programmer bug.\n");
		exit(1);
	}

	return map_sym_idx(*p_v_map, sym_name);
}
