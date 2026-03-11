#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_smooth.hpp"

std::shared_ptr<SmoothStructval> structwrap(std::shared_ptr<IrGenCtx> igc, const Smooth& smooth);
