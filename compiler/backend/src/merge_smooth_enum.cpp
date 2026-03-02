#include <bit>
#include <cstdio>
#include <cstdlib>
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "merge_smooth_enum.hpp"
#include "merge_type_enum.hpp"
#include "get_underlying_type.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"

std::shared_ptr<SmoothEnum> merge_smooth_enum(
	IrGenCtx& igc,
	std::shared_ptr<SmoothEnum> enum_a,
	std::shared_ptr<SmoothEnum> enum_b
) {
	Type underlying_a = get_underlying_type(enum_a->type);
	Type underlying_b = get_underlying_type(enum_b->type);
	
	auto p_v_enum_a = std::get_if<std::shared_ptr<TypeEnum>>(&underlying_a);
	auto p_v_enum_b = std::get_if<std::shared_ptr<TypeEnum>>(&underlying_b);

	if (!p_v_enum_a || !p_v_enum_b) {
		fprintf(stderr, "smooth enum should resolve to an underlying enum!\n");
		exit(1);
	}

	std::shared_ptr<TypeEnum> v_enum_merged = merge_type_enum(*p_v_enum_a, *p_v_enum_b);

	llvm::Value* merged_value = nullptr;

	// as merging enums will cause indies to change, we have to recompute here.
	// really I need to find a unified place for producing values properly, but for now this will do.
	
	if (v_enum_merged->hardsym.has_value()) {
		const std::string& hardsym = v_enum_merged->hardsym.value();
		auto it = std::find(v_enum_merged->syms.begin(), v_enum_merged->syms.end(), hardsym);
		
		uint32_t enum_idx = (uint32_t) std::distance(v_enum_merged->syms.begin(), it);
		uint32_t bit_width = (uint32_t) std::bit_width(v_enum_merged->syms.size());
		llvm::Type* int_type = llvm::IntegerType::get(igc.context, bit_width);
		merged_value = llvm::ConstantInt::get(int_type, enum_idx);
	}

	return std::make_shared<SmoothEnum>(SmoothEnum{
		Type(v_enum_merged),
		merged_value,
	});
}
