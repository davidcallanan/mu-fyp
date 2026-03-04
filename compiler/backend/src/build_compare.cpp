#include <cstdio>
#include <cstdlib>
#include <string>
#include "build_compare.hpp"
#include "build_compare_int.hpp"
#include "build_compare_float.hpp"
#include "build_compare_struct.hpp"

llvm::Value* build_compare(IrGenCtx& igc, llvm::Value* a, llvm::Value* b, const std::string& operator_) {
	llvm::Type* type = a->getType();

	if (type->isIntegerTy()) {
		return build_compare_int(igc, a, b, operator_);
	}

	if (type->isFloatingPointTy()) {
		return build_compare_float(igc, a, b, operator_);
	}

	if (type->isStructTy()) {
		return build_compare_struct(igc, a, b, operator_);
	}

	fprintf(stderr, "Cannot compare something that is not numeric or structish\n");
	exit(1);
}
