#include <cstdio>
#include <cstdlib>
#include <variant>
#include <memory>
#include "llvm/IR/BasicBlock.h"
#include "process_map_body.hpp"
#include "evaluate_smooth.hpp"
#include "t_instructions.hpp"
#include "t_smooth.hpp"
#include "llvm_value.hpp"
#include "llvm_flexi_type.hpp"
#include "smooth_type.hpp"
#include "create_value_symbol_table.hpp"
#include "is_type_singletonish.hpp"

void process_map_body(
	IrGenCtx& igc,
	const TypeMap& body
) {
	for (const auto& instruction : body.execution_sequence) {
		if (auto p_v_expr = std::get_if<std::shared_ptr<InstructionExpr>>(&instruction)) {
			const auto& v_expr = *p_v_expr;
			evaluate_smooth(igc, *v_expr->expr);
		}
		
		if (auto p_v_sym = std::get_if<std::shared_ptr<InstructionSym>>(&instruction)) {
			const auto& v_sym = *p_v_sym;
			
			std::string map_sym_var_name = "ms_" + v_sym->name;

			// we need to evaluate early here to ensure underlying_types are populated.
			// this may produce dead-code if the singletonish optimization is applied later, but llvm should eliminate this dead-code for us.
			Smooth smooth = evaluate_smooth(igc, *v_sym->typeval);
			Type discovered_type = smooth_type(smooth);

			ValueSymbolTableEntry entry = [&]() {
				if (is_type_singletonish(discovered_type)) {
					return ValueSymbolTableEntry{
						nullptr,
						nullptr,
						discovered_type,
						false, // syms, when treated as variables, are always immutable.
						std::nullopt,
					};
				}

				std::string scoped_alloca_name = igc.value_table->scope_id() + "~" + map_sym_var_name;
				llvm::Value* value = llvm_value(smooth);
				llvm::Value* alloca = igc.builder.CreateAlloca(value->getType(), nullptr, scoped_alloca_name);
				igc.builder.CreateStore(value, alloca);

				return ValueSymbolTableEntry{
					alloca,
					value->getType(),
					discovered_type, // maybe in the future we'll change how this works
					false, // syms, when treated as variables, are always immutable.
					std::holds_alternative<std::shared_ptr<SmoothMapReference>>(smooth)
						? std::optional<Smooth>(smooth)
						: std::nullopt
					,
				};
			}();
			
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
			new_igc.block_break = block_end;

			process_map_body(new_igc, *v_for->body);

			new_igc.builder.CreateBr(block_start);
			igc.builder.SetInsertPoint(block_end);
		}

		if (auto p_v_if = std::get_if<std::shared_ptr<InstructionIf>>(&instruction)) {
			const auto& v_if = *p_v_if;

			llvm::Function* current_func = igc.builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* block_end = llvm::BasicBlock::Create(igc.context, "if_END", current_func);

			bool fallthrough_done = false;

			for (size_t i = 0; i < v_if->branches.size(); i++) {
				const auto& branch = v_if->branches[i];

				llvm::BasicBlock* block_then = llvm::BasicBlock::Create(igc.context, "if_BRANCH", current_func);

				if (branch.condition != nullptr) { // exact condition given (each condition is a fallthrough so this may be an "if" or an "else if")
					Smooth condition_smooth = evaluate_smooth(igc, *branch.condition);

					if (!std::get_if<std::shared_ptr<SmoothEnum>>(&condition_smooth)) {
						fprintf(stderr, "the if statement must take in an enum, please convert your data to a bool!\n");
						exit(1);
					}

					llvm::Value* condition_value = llvm_value(condition_smooth);

					if (!condition_value->getType()->isIntegerTy(1)) {
						fprintf(stderr, "\"if\" must take an enum of 1 bit of size (a bool), not some other enum.\n");
						exit(1);
					}

					llvm::BasicBlock* block_next = llvm::BasicBlock::Create(igc.context, "if_PROGRESS", current_func);
					igc.builder.CreateCondBr(condition_value, block_then, block_next);
					igc.builder.SetInsertPoint(block_then);

					std::shared_ptr<ValueSymbolTable> inner_value_table = std::make_shared<ValueSymbolTable>(
						create_value_symbol_table(igc.value_table.get())
					);

					IrGenCtx fresh_igc = igc;
					fresh_igc.value_table = inner_value_table;

					process_map_body(fresh_igc, *branch.body);

					fresh_igc.builder.CreateBr(block_end);
					igc.builder.SetInsertPoint(block_next);
				} else { // no condition means fallthrough, effectively means "else" or "if (true)" in the context of "if" statements.
					igc.builder.CreateBr(block_then);
					igc.builder.SetInsertPoint(block_then);

					std::shared_ptr<ValueSymbolTable> brand_new_value_table = std::make_shared<ValueSymbolTable>(
						create_value_symbol_table(igc.value_table.get())
					);

					IrGenCtx inner_igc = igc;
					inner_igc.value_table = brand_new_value_table;

					process_map_body(inner_igc, *branch.body);

					inner_igc.builder.CreateBr(block_end);
					
					fallthrough_done = true;
					
					break;
				}
			}

			if (!fallthrough_done) {
				igc.builder.CreateBr(block_end);
			}
			
			igc.builder.SetInsertPoint(block_end);
		}
		
		if (auto p_v_break = std::get_if<std::shared_ptr<InstructionBreak>>(&instruction)) {
			if (igc.block_break == nullptr) {
				fprintf(stderr, "You are only permitted to use break when a break target is known (i.e. inside a for loop)\n");
				exit(1);
			}

			igc.builder.CreateBr(igc.block_break);

			llvm::Function* current_func = igc.builder.GetInsertBlock()->getParent();
			llvm::BasicBlock* deadblock = llvm::BasicBlock::Create(igc.context, "__DeadBlock__", current_func); // the idea behind this is to have a block in case the user writes some code after the break. but the code would be dead code and eliminator by the optimizer.
			
			igc.builder.SetInsertPoint(deadblock);
			
			return;
		}
	}
}
