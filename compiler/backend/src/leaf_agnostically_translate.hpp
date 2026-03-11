#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types.hpp"

Smooth leaf_agnostically_translate(std::shared_ptr<IrGenCtx> igc, Smooth smooth, std::shared_ptr<TypeMap> target_map, bool use_flexi_mode = false);
