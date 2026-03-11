#pragma once

#include <string>
#include "llvm/IR/Value.h"
#include "t_types_fwd.hpp"
#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_smooth.hpp"

Smooth evaluate_smooth(
	std::shared_ptr<IrGenCtx> igc,
	const Type& type
);
