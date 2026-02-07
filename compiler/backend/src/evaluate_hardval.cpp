#include <cstdio>
#include <cstdlib>
#include <variant>
#include "evaluate_hardval.hpp"
#include "t_hardval.hpp"

llvm::Value* evaluate_hardval( // for now just string evaluatoin, will refactor everything into this at a later stage.
	IrGenCtx& igc,
	const Hardval& hardval
) {
	if (std::holds_alternative<std::shared_ptr<HardvalString>>(hardval)) {
		const auto& p_v_string = std::get<std::shared_ptr<HardvalString>>(hardval);
		llvm::Value* str_const = igc.builder.CreateGlobalStringPtr(p_v_string->value);
		return str_const;
	}
	
	if (std::holds_alternative<std::shared_ptr<HardvalVarAccess>>(hardval)) {
		const auto& p_v_var_access = std::get<std::shared_ptr<HardvalVarAccess>>(hardval);
		std::optional<llvm::Value*> o_source_alloca = igc.value_table.get(p_v_var_access->target_name);
		
		if (!o_source_alloca.has_value()) {
			fprintf(stderr, "This variable %s was not actually present in our value table\n", p_v_var_access->target_name.c_str());
			exit(1);
		}
		
		llvm::Value* source_alloca = o_source_alloca.value();
		llvm::Type* desired_type = llvm::cast<llvm::AllocaInst>(source_alloca)->getAllocatedType();
		llvm::Value* loaded_value = igc.builder.CreateLoad(desired_type, source_alloca);
		
		return loaded_value;
	}
	
	fprintf(stderr, "Unhandled evaluation logic for hardval\n");
	exit(1);
}
