#pragma once

#include <optional>
#include "llvm/IR/Value.h"
#include "t_types_fwd.hpp"
#include "t_smooth_fwd.hpp"

struct SmoothStructval {
	Type type;
	llvm::Value* value;
	bool has_leaf;
	std::optional<Smooth> leaf;
};

struct SmoothPointer {
	Type type;
	llvm::Value* value;
};

struct SmoothEnum {
	Type type;
	llvm::Value* value;
};

struct SmoothInt {
	Type type;
	llvm::Value* value;
};

struct SmoothFloat {
	Type type;
	llvm::Value* value;
};

struct SmoothVoid {
	llvm::Value* value;
};
