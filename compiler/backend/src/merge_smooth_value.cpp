#include <cstdio>
#include <cstdlib>
#include <variant>
#include "merge_smooth_value.hpp"
#include "merge_type_map.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"

std::shared_ptr<SmoothStructval> merge_smooth_structval(
	IrGenCtx& igc,
	std::shared_ptr<SmoothStructval> structval_a,
	std::shared_ptr<SmoothStructval> structval_b
) {
	auto p_v_map_a = std::get_if<std::shared_ptr<TypeMap>>(&structval_a->type);
	auto p_v_map_b = std::get_if<std::shared_ptr<TypeMap>>(&structval_b->type);
	
	if (!p_v_map_a || !p_v_map_b) {
		fprintf(stderr, "only makes sense to merge two maps for now, really i don't think there should ever be non-map as a smooth_value.\n");
		exit(1);
	}

	std::shared_ptr<TypeMap> merged = merge_type_map(*p_v_map_a, *p_v_map_b);

	return std::make_shared<SmoothStructval>(SmoothStructval{
		merged,
		structval_b->value,
		structval_b->has_leaf,
	});
}
