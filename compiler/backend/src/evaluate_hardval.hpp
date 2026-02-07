#pragma once

#include <string>
#include "llvm/IR/Value.h"
#include "t_hardval_fwd.hpp"
#include "t_ctx.hpp"

llvm::Value* evaluate_hardval(
	IrGenCtx& igc,
	const Hardval& hardval,
	const std::string& type_str = ""
);
