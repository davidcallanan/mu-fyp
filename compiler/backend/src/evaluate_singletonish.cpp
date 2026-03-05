#include <bit>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "evaluate_singletonish.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_hardval.hpp"
#include "preinstantiated_smooths.hpp"

Smooth evaluate_singletonish(IrGenCtx& igc, Type type) {
	if (auto p_v_void = std::get_if<std::shared_ptr<TypeVoid>>(&type)) {
		return smooth_void(igc, type);
	}

	if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&type)) {
		auto v_enum = *p_v_enum;

		if (!v_enum->hardsym.has_value()) {
			fprintf(stderr, "Enum was noted as singletonish incorrectly - no hardsym.\n");
			exit(1);
		}

		const std::string& hardsym = v_enum->hardsym.value();
		auto it = std::find(v_enum->syms.begin(), v_enum->syms.end(), hardsym);
		uint32_t enum_idx = (uint32_t) std::distance(v_enum->syms.begin(), it);
		uint32_t bit_width = (uint32_t) std::bit_width(v_enum->syms.size() - 1);
		llvm::Type* int_type = llvm::IntegerType::get(igc.context, bit_width);
		llvm::Constant* value_as_constant = llvm::ConstantInt::get(int_type, enum_idx);

		return std::make_shared<SmoothEnum>(SmoothEnum{
			type,
			value_as_constant,
		});
	}

	if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
		auto v_pointer = *p_v_pointer;

		if (!v_pointer->hardval.has_value()) {
			fprintf(stderr, "Pointer was incorrectly noted as singletonish when no hardval.\n");
			exit(1);
		}

		if (auto p_v_str = std::get_if<std::shared_ptr<HardvalString>>(&v_pointer->hardval.value())) {
			llvm::Constant* str_const = igc.builder.CreateGlobalStringPtr((*p_v_str)->value);
			return std::make_shared<SmoothPointer>(SmoothPointer{ type, str_const });
		}

		fprintf(stderr, "Pointer hardval not implemented for singletonish optimization.\n");
		exit(1);
	}

	fprintf(stderr, "Type didn't have any singletonishability.\n");
	exit(1);
}
