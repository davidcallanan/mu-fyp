#include "merge_smooth_float.hpp"

#include <cstdio>
#include <cstdlib>
#include "get_underlying_type.hpp"
#include "rotten_float_info.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"

std::shared_ptr<SmoothFloat> merge_smooth_float(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothFloat> float_a,
	std::shared_ptr<SmoothFloat> float_b
) {
	Type underlying_a = get_underlying_type(float_a->type);
	Type underlying_b = get_underlying_type(float_b->type);

	auto p_v_rotten_a = std::get_if<std::shared_ptr<TypeRotten>>(&underlying_a);
	auto p_v_rotten_b = std::get_if<std::shared_ptr<TypeRotten>>(&underlying_b);

	if (!p_v_rotten_a || !p_v_rotten_b) {
		fprintf(stderr, "Bizzare situation.\n");
		exit(1);
	}

	auto info_a = rotten_float_info(*p_v_rotten_a);
	auto info_b = rotten_float_info(*p_v_rotten_b);

	if (!info_a || !info_b) {
		fprintf(stderr, "Bizzare situation\n");
		exit(1);
	}

	uint32_t bits_a = info_a->bits;
	uint32_t bits_b = info_b->bits;

	if (float_a->value != nullptr && float_b->value != nullptr) {
		fprintf(stderr, "More than one float encountuered while merging smooths.\n");
		exit(1);
	}

	if (float_a->value == nullptr && float_b->value == nullptr) {
		return bits_a >= bits_b ? float_a : float_b;
	}

	llvm::Value* value = float_a->value != nullptr ? float_a->value : float_b->value;

	return std::make_shared<SmoothFloat>(SmoothFloat{
		bits_a >= bits_b ? float_a->type : float_b->type,
		value,
	});
}
