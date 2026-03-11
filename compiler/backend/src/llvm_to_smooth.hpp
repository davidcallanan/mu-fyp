#pragma once

#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types_fwd.hpp"
#include "llvm/IR/Value.h"

Smooth llvm_to_smooth(std::shared_ptr<IrGenCtx> igc, const Type& type, llvm::Value* raw);
