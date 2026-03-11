#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"

std::shared_ptr<SmoothEnum> merge_smooth_enum(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothEnum> enum_a,
	std::shared_ptr<SmoothEnum> enum_b
);
