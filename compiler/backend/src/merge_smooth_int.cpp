#include "merge_smooth_int.hpp"

#include <cstdio>
#include <cstdlib>
#include "get_underlying_type.hpp"
#include "rotten_int_info.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "llvm/IR/DerivedTypes.h"

std::shared_ptr<SmoothInt> merge_smooth_int(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothInt> int_a,
	std::shared_ptr<SmoothInt> int_b
) {
	Type underlying_a = get_underlying_type(int_a->type);
	Type underlying_b = get_underlying_type(int_b->type);

	auto p_v_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(&underlying_a);
	auto p_v_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(&underlying_b);

	if (!p_v_rotten_a || !p_v_rotten_b) {
		fprintf(stderr, "Invariant glitch.\n");
		exit(1);
	}

	auto info_a = rotten_int_info(*p_v_rotten_a);
	auto info_b = rotten_int_info(*p_v_rotten_b);

	if (!info_a || !info_b) {
		fprintf(stderr, "Another invariant glitch.\n");
		exit(1);
	}

	uint32_t bits_a = info_a->bits;
	uint32_t bits_b = info_b->bits;

	if (int_a->value != nullptr && int_b->value != nullptr) {
		fprintf(stderr, "Failed to merge two smooths because more than one int value.\n");
		exit(1);
	}
	
	if (int_a->value == nullptr && int_b->value == nullptr) {
		return bits_a >= bits_b ? int_a : int_b;
	}

	llvm::Value* value = int_a->value != nullptr ? int_a->value : int_b->value;
	llvm::Type* type = llvm::IntegerType::get(*igc->context, bits_a >= bits_b ? bits_a : bits_b);
	value = igc->builder->CreateZExt(value, type);

	return std::make_shared<SmoothInt>(SmoothInt{
		bits_a >= bits_b ? int_a->type : int_b->type,
		value,
	});
}
