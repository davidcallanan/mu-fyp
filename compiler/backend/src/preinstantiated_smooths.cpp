#include "preinstantiated_smooths.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "t_smooth.hpp"

std::shared_ptr<SmoothVoid> smooth_void(IrGenCtx& igc) {
	llvm::StructType* struct_type = llvm::StructType::get(igc.context, {});
	llvm::Value* value = llvm::UndefValue::get(struct_type);
	
	return std::make_shared<SmoothVoid>(SmoothVoid{
		value,
	});
}
