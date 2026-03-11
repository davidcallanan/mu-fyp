#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types.hpp"

Smooth better_leaf_agnostically_translate(std::shared_ptr<IrGenCtx> igc, Smooth smooth, const Type& target_type, bool use_flexi_mode = false);
