#include "build_compare_int.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

llvm::Value* build_compare_int(std::shared_ptr<IrGenCtx> igc, llvm::Value* a, llvm::Value* b, const std::string& operator_) {
	// todo: what about distinguishing between signed and unsigned.
	
	uint32_t a_bits = a->getType()->getIntegerBitWidth();
	uint32_t b_bits = b->getType()->getIntegerBitWidth();

	if (a_bits < b_bits) {
		a = igc->builder->CreateZExt(a, b->getType());
	} else if (b_bits < a_bits) {
		b = igc->builder->CreateZExt(b, a->getType());
	}

	if (operator_ == "==") return igc->builder->CreateICmpEQ(a, b);
	if (operator_ == "!=") return igc->builder->CreateICmpNE(a, b);
	if (operator_ == "<")  return igc->builder->CreateICmpULT(a, b);
	if (operator_ == ">")  return igc->builder->CreateICmpUGT(a, b);
	if (operator_ == "<=") return igc->builder->CreateICmpULE(a, b);
	if (operator_ == ">=") return igc->builder->CreateICmpUGE(a, b);

	fprintf(stderr, "Unhandled operator %s\n", operator_.c_str());
	exit(1);
}
