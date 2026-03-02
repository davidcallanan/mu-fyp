#include "llvm_to_smooth_bool.hpp"
#include "evaluate_structval.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"

std::shared_ptr<SmoothEnum> llvm_to_smooth_bool(
	IrGenCtx& igc,
	llvm::Value* value
) {
	auto enum_false = std::make_shared<TypeEnum>();
	enum_false->syms = {"false", "true"};
	enum_false->hardsym = "false";
	enum_false->is_instantiated = true;

	auto enum_true = std::make_shared<TypeEnum>();
	enum_true->syms = {"false", "true"};
	enum_true->hardsym = "true";
	enum_true->is_instantiated = true;

	auto enum_bool = std::make_shared<TypeEnum>();
	enum_bool->syms = {"false", "true"};
	enum_bool->is_instantiated = true;

	auto smooth_enum_false = std::get<std::shared_ptr<SmoothEnum>>(evaluate_smooth(igc, std::shared_ptr<TypeEnum>(enum_false)));
	auto smooth_enum_true = std::get<std::shared_ptr<SmoothEnum>>(evaluate_smooth(igc, std::shared_ptr<TypeEnum>(enum_true)));
	auto smooth_enum_bool = std::get<std::shared_ptr<SmoothEnum>>(evaluate_smooth(igc, std::shared_ptr<TypeEnum>(enum_bool)));

	llvm::Value* of_interest = igc.builder.CreateSelect(value, smooth_enum_true->value, smooth_enum_false->value);

	return std::make_shared<SmoothEnum>(SmoothEnum{
		smooth_enum_bool->type,
		of_interest,
	});
}
