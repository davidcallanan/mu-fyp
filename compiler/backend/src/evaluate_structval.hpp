#pragma once

#include <string>
#include "llvm/IR/Value.h"
#include "t_types_fwd.hpp"
#include "t_ctx.hpp"
#include "t_smooth_value.hpp"

SmoothValue evaluate_structval(
	IrGenCtx& igc,
	const Type& type
);
