#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types_fwd.hpp"

#include "llvm/IR/Value.h"

Smooth fresh_smooth(IrGenCtx& igc, const Smooth& old_smooth, llvm::Value* fresh_value);
