#include <cstdio>
#include <cstdlib>
#include <string>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "build_compare_struct.hpp"
#include "build_compare.hpp"

// this is probably really inefficient, but i don't have time to worry.

llvm::Value* build_compare_struct(std::shared_ptr<IrGenCtx> igc, llvm::Value* a, llvm::Value* b, const std::string& operator_) {
	llvm::StructType* struct_type = llvm::cast<llvm::StructType>(a->getType());
	unsigned num_fields = struct_type->getNumElements();

	llvm::Type* i1 = llvm::Type::getInt1Ty(*igc->context);

	if (operator_ == "==" || operator_ == "!=") {
		llvm::Value* result = llvm::ConstantInt::get(i1, 1);

		for (unsigned i = 0; i < num_fields; i++) {
			llvm::Value* field_a = igc->builder->CreateExtractValue(a, i);
			llvm::Value* field_b = igc->builder->CreateExtractValue(b, i);
			
			result = igc->builder->CreateAnd(result, build_compare(igc, field_a, field_b, "=="));
		}
		
		if (operator_ == "!=") {
			return igc->builder->CreateNot(result);
		}
		
		return result;
	}
	
	// we implement lexicographical ordering here.

	bool should_tiebreak = operator_ == "<=" || operator_ == ">="; // if not strict, then equality is a tiebreaker.
	llvm::Value* result = llvm::ConstantInt::get(i1, should_tiebreak ? 1 : 0);
	std::string op_strict = operator_ == "<" || operator_ == "<=" ? "<" : ">";

	// i'm trusting that the llvm optimizer pass will convert this to a binary tree or something else smart.
	// this is only a prototype compiler - optimizations like this can be hand-written at a later stage if neccessary.
	
	for (unsigned i = num_fields - 1; i < num_fields; i--) { // unsigned underflow intentionally handled.
		llvm::Value* field_a = igc->builder->CreateExtractValue(a, i);
		llvm::Value* field_b = igc->builder->CreateExtractValue(b, i);
		llvm::Value* comp_strictful = build_compare(igc, field_a, field_b, op_strict);
		llvm::Value* comp_equaliful = build_compare(igc, field_a, field_b, "==");
		
		result = igc->builder->CreateSelect(
			comp_strictful,
			llvm::ConstantInt::get(i1, 1),
			igc->builder->CreateSelect(
				comp_equaliful,
				result,
				llvm::ConstantInt::get(i1, 0)
			)
		);
	}

	return result;
}
