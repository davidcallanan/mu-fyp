#include <cstdlib>
#include "t_smooth_value.hpp"

llvm::Value* SmoothValue::extract_leaf(llvm::IRBuilder<>& builder) const {
	if (!has_leaf) {
		fprintf(stderr, "Bad programmer - check for .has_leaf first!\n");
		exit(1);
	}
	
	return builder.CreateExtractValue(struct_value, 0);
}
