#pragma once

#include "llvm/IR/Value.h"
#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"

std::shared_ptr<SmoothEnum> llvm_to_smooth_bool(
	IrGenCtx& igc,
	llvm::Value* i1_val
);
