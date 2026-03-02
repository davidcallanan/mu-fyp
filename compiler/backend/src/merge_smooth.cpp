#include "merge_smooth.hpp"
#include "merge_smooth_value.hpp"
#include "merge_smooth_enum.hpp"
#include "t_smooth.hpp"

Smooth merge_smooth(IrGenCtx& igc, Smooth smooth_a, Smooth smooth_b) {
	auto p_v_structval_a = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth_a);
	auto p_v_structval_b = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth_b);

	if (p_v_structval_a && p_v_structval_b) {
		return merge_smooth_structval(igc, *p_v_structval_a, *p_v_structval_b);
	}

	auto p_v_enum_a = std::get_if<std::shared_ptr<SmoothEnum>>(&smooth_a);
	auto p_v_enum_b = std::get_if<std::shared_ptr<SmoothEnum>>(&smooth_b);

	if (p_v_enum_a && p_v_enum_b) {
		return merge_smooth_enum(igc, *p_v_enum_a, *p_v_enum_b);
	}

	fprintf(stderr, "Only structvals and enums can be merged, you must be merging something exotic which wouldn't make sense.\n");
	exit(1);
}
