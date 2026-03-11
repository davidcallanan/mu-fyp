#include "force_identical_layout.hpp"
#include "t_ctx.hpp"
#include <cstdio>
#include <cstdlib>

llvm::Value* force_identical_layout(std::shared_ptr<IrGenCtx> igc, llvm::Value* value, llvm::Type* target_type) {
	auto* type = llvm::dyn_cast<llvm::StructType>(value->getType());

	if (!type) {
		if (value->getType() != target_type) {
			fprintf(stderr, "did not pass in something that was a struct or of matching type.\n");
			fprintf(stderr, "got type details: ");
			value->getType()->print(llvm::errs());
			fprintf(stderr, ".. wanted: ");
			target_type->print(llvm::errs());
			fprintf(stderr, "\n");
			exit(1);
		}
		
		return value;
	}

	if (type == target_type) {
		return value;
	}

	auto* target_struct = llvm::dyn_cast<llvm::StructType>(target_type);

	if (!target_struct) {
		fprintf(stderr, "given a struct, but wanted a non-struct target\n");
		fprintf(stderr, "input was:");
		type->print(llvm::errs());
		fprintf(stderr, "\n");
		fprintf(stderr, "target was:");
		target_type->print(llvm::errs());
		fprintf(stderr, "\n");
		exit(1);
	}

	if (!target_struct->isLayoutIdentical(type)) {
		fprintf(stderr, "the types do not match: ..%s.. and ..%s..\n", type->getName().str().c_str(), target_struct->getName().str().c_str());
		exit(1);
	}

	return igc->builder->CreateBitCast(value, target_struct);
}
