#include "smooth_type.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"

Type smooth_type(Smooth smooth) {
	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		return (*p_v_structval)->type;
	}

	if (auto p_v_pointer = std::get_if<std::shared_ptr<SmoothPointer>>(&smooth)) {
		return (*p_v_pointer)->type;
	}

	if (auto p_v_enum = std::get_if<std::shared_ptr<SmoothEnum>>(&smooth)) {
		return (*p_v_enum)->type;
	}

	if (auto p_v_int = std::get_if<std::shared_ptr<SmoothInt>>(&smooth)) {
		return (*p_v_int)->type;
	}

	if (auto p_v_float = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth)) {
		return (*p_v_float)->type;
	}

	fprintf(stderr, "The particular smooth does not have a type associated with it (programmer bug?)\n");
	exit(1);
}
