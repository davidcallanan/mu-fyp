#pragma once

#include <memory>
#include "t_smooth_fwd.hpp"
#include "t_ctx.hpp"
#include "llvm/IR/Type.h"

llvm::Type* llvm_opaqued_flexi_type(const Smooth smooth, std::shared_ptr<IrGenCtx> igc);
