#pragma once

#include "t_bundles_fwd.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/DerivedTypes.h"

struct BundleMap {
	llvm::Function* call_func;
	llvm::Function* call_func_alwaysinline;
	llvm::StructType* opaque_struct_type; // this is opaque until the final struct is determined.
	// one must only use it for passing around until the final struct is known.
};
