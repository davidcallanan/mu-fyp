#pragma once

#include "t_ctx.hpp"
#include "t_smooth.hpp"

std::shared_ptr<SmoothStructval> merge_smooth_structval(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothStructval> structval_a,
	std::shared_ptr<SmoothStructval> structval_b
);
