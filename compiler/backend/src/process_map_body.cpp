#include <cstdio>
#include <cstdlib>
#include <variant>
#include <memory>
#include "llvm/IR/BasicBlock.h"
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
		if (auto p_v_expr = std::get_if<std::shared_ptr<InstructionExpr>>(&instruction)) {
			const auto& v_expr = *p_v_expr;
			evaluate_structval(igc, *v_expr->expr);
		}
		
		if (auto p_v_sym = std::get_if<std::shared_ptr<InstructionSym>>(&instruction)) {
			const auto& v_sym = *p_v_sym;
			
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
				false, // syms, when treated as variables, are always immutable.
			};
			
			igc.value_table->set(map_sym_var_name, entry);
		}

		if (auto p_v_for = std::get_if<std::shared_ptr<InstructionFor>>(&instruction)) {
			const auto& v_for = *p_v_for;

			llvm::Function* current_func = igc.builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* block_start = llvm::BasicBlock::Create(igc.context, "for_START", current_func);
			llvm::BasicBlock* block_end = llvm::BasicBlock::Create(igc.context, "for_END", current_func);

			igc.builder.CreateBr(block_start);
			igc.builder.SetInsertPoint(block_start);

			std::shared_ptr<ValueSymbolTable> new_value_table = std::make_shared<ValueSymbolTable>(
				create_value_symbol_table(igc.value_table.get())
			);

			IrGenCtx new_igc = igc;
			new_igc.value_table = new_value_table;

			process_map_body(new_igc, *v_for->body);

			new_igc.builder.CreateBr(block_start);
			igc.builder.SetInsertPoint(block_end);
		}
	}
}
