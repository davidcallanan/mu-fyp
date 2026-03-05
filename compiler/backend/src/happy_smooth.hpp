#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types_fwd.hpp"

// the purpose behind this is keeping the smooth value happy, keeping it in line with the type information.
// the idea is that the underlying_type is the source of truth, but proper type information might be higher up the hierarchy.
// so every so often we run our smooths through happy_smooth if there is a risk of a mismatch.
// at the moment, the only risk of mismatch is during void type merging, so it is only called there.
Smooth happy_smooth(IrGenCtx& igc, Smooth smooth, const Type& type);
