#pragma once

#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "t_types_fwd.hpp"

class SmoothValue {
public:
	llvm::Value* struct_value;
	Type type;
	bool has_leaf;
	
	llvm::Value* extract_leaf(llvm::IRBuilder<>& builder) const;
};
