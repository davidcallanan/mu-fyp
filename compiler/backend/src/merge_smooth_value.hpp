#pragma once

#include "t_ctx.hpp"
#include "t_smooth_value.hpp"

SmoothValue merge_smooth_value(
	IrGenCtx& igc,
	const SmoothValue& smooth_a,
	const SmoothValue& smooth_b
);
