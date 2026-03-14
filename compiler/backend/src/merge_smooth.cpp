#include <cstdio>
#include <cstdlib>
#include "merge_smooth.hpp"
#include "merge_smooth_value.hpp"
#include "merge_smooth_enum.hpp"
#include "merge_smooth_int.hpp"
#include "merge_smooth_float.hpp"
#include "merge_smooth_void_voidptr_with_map_reference.hpp"
#include "merge_smooth_void_map_reference_with_voidptr.hpp"
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

	if (auto p_void_voidptr_a = std::get_if<std::shared_ptr<SmoothVoidVoidptr>>(&smooth_a)) {
		if (auto p_voidptr_b = std::get_if<std::shared_ptr<SmoothVoidptr>>(&smooth_b)) {
			return smooth_b;
		}

		if (auto p_map_reference_b = std::get_if<std::shared_ptr<SmoothMapReference>>(&smooth_b)) {
			return merge_smooth_void_voidptr_with_map_reference(igc, *p_void_voidptr_a, *p_map_reference_b);
		}

		fprintf(stderr, "A non-definitive void pointer cannot be merged with something that is not a definitive map reference!\n");
		exit(1);
	}

	if (auto p_void_map_reference_a = std::get_if<std::shared_ptr<SmoothVoidMapReference>>(&smooth_a)) {
		if (auto p_voidptr_b = std::get_if<std::shared_ptr<SmoothVoidptr>>(&smooth_b)) {
			return merge_smooth_void_map_reference_with_voidptr(igc, *p_void_map_reference_a, *p_voidptr_b);
		}

		fprintf(stderr, "A non-definitive map reference cannot be merged with something that is not a definitive void piotner!\n");
		exit(1);
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
