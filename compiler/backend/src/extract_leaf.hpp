#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"

Smooth extract_leaf(std::shared_ptr<IrGenCtx> igc, Smooth smooth, bool be_permissive = false);
