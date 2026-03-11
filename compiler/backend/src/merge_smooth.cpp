#include <cstdio>
#include <cstdlib>
#include "merge_smooth.hpp"
#include "merge_smooth_value.hpp"
#include "merge_smooth_enum.hpp"
#include "merge_smooth_int.hpp"
#include "merge_smooth_float.hpp"
#include "t_smooth.hpp"

Smooth merge_smooth(std::shared_ptr<IrGenCtx> igc, Smooth smooth_a, Smooth smooth_b) {
	// todo: i dunno how we are gonna handle merging the types of void
	
	if (auto p_v_void_a = std::get_if<std::shared_ptr<SmoothVoid>>(&smooth_a)) {
		return smooth_b;
	}

	if (auto p_v_void_b = std::get_if<std::shared_ptr<SmoothVoid>>(&smooth_b)) {
		return smooth_a;
	}

	if (std::get_if<std::shared_ptr<SmoothVoidInt>>(&smooth_a)) {
		return smooth_b;
	}

	if (std::get_if<std::shared_ptr<SmoothVoidInt>>(&smooth_b)) {
		return smooth_a;
	}

	if (std::get_if<std::shared_ptr<SmoothVoidFloat>>(&smooth_a)) {
		return smooth_b;
	}

	if (std::get_if<std::shared_ptr<SmoothVoidFloat>>(&smooth_b)) {
		return smooth_a;
	}

	if (std::get_if<std::shared_ptr<SmoothVoidPointer>>(&smooth_a)) {
		return smooth_b;
	}

	if (std::get_if<std::shared_ptr<SmoothVoidPointer>>(&smooth_b)) {
		return smooth_a;
	}

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

	auto p_v_int_a = std::get_if<std::shared_ptr<SmoothInt>>(&smooth_a);
	auto p_v_int_b = std::get_if<std::shared_ptr<SmoothInt>>(&smooth_b);

	if (p_v_int_a && p_v_int_b) {
		return merge_smooth_int(igc, *p_v_int_a, *p_v_int_b);
	}

	auto p_v_float_a = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth_a);
	auto p_v_float_b = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth_b);

	if (p_v_float_a && p_v_float_b) {
		return merge_smooth_float(igc, *p_v_float_a, *p_v_float_b);
	}

	fprintf(stderr, "Only structvals and enums can be merged (and ints and floats for zero extension purposes only), you must be merging something exotic which wouldn't make sense.\n");
	exit(1);
}
