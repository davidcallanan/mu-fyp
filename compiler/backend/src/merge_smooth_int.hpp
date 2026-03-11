#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"

std::shared_ptr<SmoothInt> merge_smooth_int(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothInt> int_a,
	std::shared_ptr<SmoothInt> int_b
);
