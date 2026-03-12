#pragma once

#include <functional>
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
	std::function<llvm::Function*()> call_func;
	std::function<llvm::Function*()> call_func_alwaysinline;
	std::vector<Smooth> field_smooths;
	std::optional<llvm::Value*> intended_this;
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

struct SmoothVoidPointer {
	Type type;
	llvm::Type* flexi_type;
	llvm::Value* value;
};

struct SmoothMapReference {
	Type type;
	llvm::Value* value;
	llvm::Type* structval_type;
};
