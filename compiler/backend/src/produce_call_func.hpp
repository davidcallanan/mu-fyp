#pragma once

#include "llvm/IR/Function.h"
#include "t_ctx.hpp"
#include "t_types.hpp"

llvm::Function* produce_call_func(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<TypeMap> map,
	bool is_alwaysinline = false
);
