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
		
		if (std::holds_alternative<std::shared_ptr<InstructionLog>>(instruction)) {
			const auto& v_log = std::get<std::shared_ptr<InstructionLog>>(instruction);
			
			if (v_log->message == nullptr) {
				llvm::Value* log_str = igc.builder.CreateGlobalStringPtr("");
				igc.builder.CreateCall(igc.puts_func, { log_str });
			} else {
				SmoothValue smooth = evaluate_structval(igc, *v_log->message);
				
				if (!smooth.has_leaf) {
					fprintf(stderr, "Not good circumstances - no leaf.\n");
					exit(1);
				}
				
				llvm::Value* leaf = smooth.extract_leaf(igc.builder);
				igc.builder.CreateCall(igc.puts_func, { leaf });
			}
		}
		
		if (std::holds_alternative<std::shared_ptr<InstructionAssign>>(instruction)) {
			const auto& v_assign = std::get<std::shared_ptr<InstructionAssign>>(instruction);
			
			std::string map_var_name = "m_" + v_assign->name;
			
			SmoothValue smooth = evaluate_structval(igc, *v_assign->typeval);
			llvm::Value* alloca = igc.builder.CreateAlloca(smooth.struct_value->getType(), nullptr, map_var_name);
			igc.builder.CreateStore(smooth.struct_value, alloca);
			
			ValueSymbolTableEntry entry{
				alloca,
				smooth.struct_value->getType(),
				smooth.type,
				smooth.has_leaf,
			};
			
			igc.value_table.set(map_var_name, entry);
		}
		
		if (std::holds_alternative<std::shared_ptr<InstructionSym>>(instruction)) {
			const auto& v_sym = std::get<std::shared_ptr<InstructionSym>>(instruction);
			
			std::string map_sym_var_name = "ms_" + v_sym->name;
			
			SmoothValue smooth = evaluate_structval(igc, *v_sym->typeval);
			llvm::Value* alloca = igc.builder.CreateAlloca(smooth.struct_value->getType(), nullptr, map_sym_var_name);
			igc.builder.CreateStore(smooth.struct_value, alloca);
			
			ValueSymbolTableEntry entry{
				alloca,
				smooth.struct_value->getType(),
				smooth.type,
				smooth.has_leaf,
			};
			
			igc.value_table.set(map_sym_var_name, entry);
		}
	}
}
