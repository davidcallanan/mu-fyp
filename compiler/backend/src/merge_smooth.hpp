#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"

Smooth merge_smooth(std::shared_ptr<IrGenCtx> igc, Smooth smooth_a, Smooth smooth_b);
