#pragma once

#include "t_ctx.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Value.h"

llvm::Value* force_identical_layout(std::shared_ptr<IrGenCtx> igc, llvm::Value* value, llvm::Type* target_type);
