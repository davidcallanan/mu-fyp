#pragma once

#include <optional>
#include <vector>
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "t_types_fwd.hpp"
#include "t_smooth_fwd.hpp"

struct SmoothStructval {
	Type type;
	llvm::Value* value;
	bool has_leaf;
	std::optional<Smooth> leaf;
	llvm::Function* call_func;
	std::vector<Smooth> field_smooths;
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
	std::optional<Type> type;
	llvm::Value* value;
};

struct SmoothVoidInt {
	Type type;
	llvm::Type* flexi_type;
	llvm::Value* value;
};

struct SmoothVoidFloat {
	Type type;
	llvm::Type* flexi_type;
	llvm::Value* value;
};
