#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types.hpp"

Smooth leaf_agnostically_translate(IrGenCtx& igc, Smooth smooth, std::shared_ptr<TypeMap> target_map);
