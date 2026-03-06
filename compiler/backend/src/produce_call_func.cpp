#include <cstdio>
#include <cstdlib>
#include <string>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "produce_call_func.hpp"
#include "t_ctx.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"
#include "evaluate_smooth.hpp"
#include "process_map_body.hpp"
#include "create_value_symbol_table.hpp"
#include "create_dummy_igc.hpp"
#include "destroy_dummy_igc.hpp"
#include "llvm_value.hpp"

llvm::Function* produce_call_func(
	IrGenCtx& igc,
	std::shared_ptr<TypeMap> map
) {
	if (false
		|| map->call_input_type == nullptr
		|| map->call_output_type == nullptr
	) {
		return nullptr;
	}

	DummyIgc dummy1 = create_dummy_igc(igc);
	Smooth input_smooth = evaluate_smooth(dummy1.igc, Type(map->call_input_type));
	destroy_dummy_igc(dummy1);

	DummyIgc dummy2 = create_dummy_igc(igc);
	TypeMap output_shell = *map->call_output_type;
	output_shell.execution_sequence.clear();
	auto v_map = std::make_shared<TypeMap>(output_shell);
	Smooth output_smooth_probe = evaluate_smooth(dummy2.igc, Type(v_map)); // if this works, what a lifehack
	destroy_dummy_igc(dummy2);

	llvm::StructType* input_struct_type = llvm::cast<llvm::StructType>(llvm_value(input_smooth)->getType());
	llvm::StructType* output_struct_type = llvm::cast<llvm::StructType>(llvm_value(output_smooth_probe)->getType());

	llvm::FunctionType* func_type = llvm::FunctionType::get(
		output_struct_type,
		{ input_struct_type },
		false
	);

	static int next_callable_id = 0;
	std::string func_name = "__ec_callable_" + std::to_string(next_callable_id++);

	llvm::Function* func = llvm::Function::Create(
		func_type,
		llvm::Function::PrivateLinkage,
		func_name,
		igc.module
	);

	llvm::BasicBlock* func_entry = llvm::BasicBlock::Create(igc.context, "entry", func);
	llvm::IRBuilder<> func_builder(igc.context);
	func_builder.SetInsertPoint(func_entry);

	std::shared_ptr<ValueSymbolTable> new_value_table = std::make_shared<ValueSymbolTable>( // currently no closure ability implemented.
		create_value_symbol_table()
	);

	IrGenCtx enhanced_igc = {
		igc.context,
		igc.module,
		func_builder,
		igc.type_table,
		new_value_table,
		igc.puts_func,
		igc.log_data_func,
		igc.log_data_deref_func,
		nullptr,
	};

	llvm::Argument* single_argument = func->getArg(0);
	single_argument->setName("InputMap");

	llvm::Value* input_alloca = func_builder.CreateAlloca(input_struct_type, nullptr, "input_struct");
	func_builder.CreateStore(single_argument, input_alloca);

	{
		if (!map->call_input_identifier.has_value()) {
			fprintf(stderr, "must take in input using entire destructure for now.\n");
			exit(1);
		}

		std::string input_var_name = "m_" + map->call_input_identifier.value();

		ValueSymbolTableEntry input_entry{
			input_alloca,
			input_struct_type,
			Type(map->call_input_type),
			false,
		};

		new_value_table->set(input_var_name, input_entry);
	}

	process_map_body(enhanced_igc, *map->call_output_type);
	Smooth output_smooth = evaluate_smooth(enhanced_igc, Type(map->call_output_type));
	func_builder.CreateRet(llvm_value(output_smooth));

	return func;
}
