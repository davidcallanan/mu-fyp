#include <cstdio>
#include <cstdlib>
#include <string>
#include "build_compare_float.hpp"

llvm::Value* build_compare_float(IrGenCtx& igc, llvm::Value* a, llvm::Value* b, const std::string& operator_) {
	if (operator_ == "==") return igc.builder.CreateFCmpOEQ(a, b);
	if (operator_ == "!=") return igc.builder.CreateFCmpONE(a, b);
	if (operator_ == "<")  return igc.builder.CreateFCmpOLT(a, b);
	if (operator_ == ">")  return igc.builder.CreateFCmpOGT(a, b);
	if (operator_ == "<=") return igc.builder.CreateFCmpOLE(a, b);
	if (operator_ == ">=") return igc.builder.CreateFCmpOGE(a, b);

	fprintf(stderr, "Bizarre operator %s\n", operator_.c_str());
	exit(1);
}
