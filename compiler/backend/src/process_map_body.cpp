#include <cstdio>
#include <cstdlib>
#include <variant>
#include <memory>
#include "process_map_body.hpp"
#include "evaluate_structval.hpp"
#include "t_instructions.hpp"
#include "t_smooth_value.hpp"
#include "create_value_symbol_table.hpp"

void process_map_body(
	IrGenCtx& igc,
	const TypeMap& body
) {
	for (const auto& instruction : body.execution_sequence) {
		// todo: need to change std::holds_alternative to std::get_if
		
		if (std::holds_alternative<std::shared_ptr<InstructionExpr>>(instruction)) {
			const auto& v_expr = std::get<std::shared_ptr<InstructionExpr>>(instruction);
			evaluate_structval(igc, *v_expr->expr);
		}
		
		if (std::holds_alternative<std::shared_ptr<InstructionSym>>(instruction)) {
			const auto& v_sym = std::get<std::shared_ptr<InstructionSym>>(instruction);
			
			std::string map_sym_var_name = "ms_" + v_sym->name;
			std::string scoped_alloca_name = igc.value_table->scope_id() + "~" + map_sym_var_name;
			
			SmoothValue smooth = evaluate_structval(igc, *v_sym->typeval);
			llvm::Value* alloca = igc.builder.CreateAlloca(smooth.struct_value->getType(), nullptr, scoped_alloca_name);
			igc.builder.CreateStore(smooth.struct_value, alloca);
			
			ValueSymbolTableEntry entry{
				alloca,
				smooth.struct_value->getType(),
				smooth.type,
				smooth.has_leaf,
			};
			
			igc.value_table->set(map_sym_var_name, entry);
		}
	}
}
