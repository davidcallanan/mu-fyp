#pragma once

#include <string>
#include "llvm/IR/Value.h"
#include "t_ctx.hpp"

llvm::Value* build_compare_int(std::shared_ptr<IrGenCtx> igc, llvm::Value* a, llvm::Value* b, const std::string& operator_);
