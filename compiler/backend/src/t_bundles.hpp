#pragma once

#include "t_bundles_fwd.hpp"
#include "llvm/IR/Function.h"

struct BundleMap {
	llvm::Function* call_func;
	llvm::Function* call_func_alwaysinline;
};
