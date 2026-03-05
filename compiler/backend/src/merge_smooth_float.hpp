#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"

std::shared_ptr<SmoothFloat> merge_smooth_float(
	IrGenCtx& igc,
	std::shared_ptr<SmoothFloat> float_a,
	std::shared_ptr<SmoothFloat> float_b
);
