#include "llvm_to_smooth_bool.hpp"
#include "evaluate_smooth.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"
#include "preinstantiated_types.hpp"

std::shared_ptr<SmoothEnum> llvm_to_smooth_bool(
	std::shared_ptr<IrGenCtx> igc,
	llvm::Value* value
) {
	auto smooth_enum_false = std::get<std::shared_ptr<SmoothEnum>>(evaluate_smooth(igc, Type(type_false)));
	auto smooth_enum_true = std::get<std::shared_ptr<SmoothEnum>>(evaluate_smooth(igc, Type(type_true)));

	llvm::Value* of_interest = igc->builder->CreateSelect(value, smooth_enum_true->value, smooth_enum_false->value);

	return std::make_shared<SmoothEnum>(SmoothEnum{
		Type(type_bool),
		of_interest,
	});
}
