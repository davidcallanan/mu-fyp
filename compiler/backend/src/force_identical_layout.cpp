#include "force_identical_layout.hpp"
#include "t_ctx.hpp"
#include <cstdio>
#include <cstdlib>

llvm::Value* force_identical_layout(std::shared_ptr<IrGenCtx> igc, llvm::Value* value, llvm::StructType* target_type) {
	auto* type = llvm::dyn_cast<llvm::StructType>(value->getType());

	if (!type) {
		fprintf(stderr, "did not pass in something that was a struct.\n");
		exit(1);
	}

	if (type == target_type) {
		return value;
	}

	if (!target_type->isLayoutIdentical(type)) {
		fprintf(stderr, "the types do not match: ..%s.. and ..%s..\n", type->getName().str().c_str(), target_type->getName().str().c_str());
		exit(1);
	}

	return igc->builder->CreateBitCast(value, target_type);
}
