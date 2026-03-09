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
#include "t_bundles.hpp"
#include "evaluate_smooth.hpp"
#include "create_value_symbol_table.hpp"
#include "create_dummy_igc.hpp"
#include "destroy_dummy_igc.hpp"
#include "llvm_value.hpp"
#include "llvm_flexi_type.hpp"

llvm::Function* produce_call_func(
	IrGenCtx& igc,
	std::shared_ptr<TypeMap> map,
	bool is_alwaysinline
) {
	if (false
		|| map->call_input_type == nullptr
		|| map->call_output_type == nullptr
	) {
		return nullptr;
	}

	if (!map->bundle_id.has_value()) {
		fprintf(stderr, "Callable maps require a designated bundle.\n");
		exit(1);	
	}
	
	Bundle* bundle = igc.toc->bundle_registry.get(map->bundle_id.value());
	
	auto p_bundle_map = std::get_if<std::shared_ptr<BundleMap>>(bundle);
	
	if (!p_bundle_map) {
		fprintf(stderr, "Bundle not from map.");
		exit(1);
	}
	
	if ((*p_bundle_map)->call_func != nullptr) {
		if (is_alwaysinline) {
			return (*p_bundle_map)->call_func_alwaysinline;
		}
		
		return (*p_bundle_map)->call_func;
	}
	
	if (is_alwaysinline) {
		return nullptr;
	}

	DummyIgc dummy1 = create_dummy_igc(igc);
	Smooth input_smooth = evaluate_smooth(dummy1.igc, Type(map->call_input_type));
	llvm::StructType* input_struct_type = llvm::cast<llvm::StructType>(llvm_flexi_type(input_smooth));
	destroy_dummy_igc(dummy1);

	DummyIgc dummy2 = create_dummy_igc(igc);
	TypeMap output_shell = *map->call_output_type;
	output_shell.execution_sequence.clear();
	auto v_map = std::make_shared<TypeMap>(output_shell);
	Smooth output_smooth_probe = evaluate_smooth(dummy2.igc, Type(v_map)); // if this works, what a lifehack
	llvm::StructType* output_struct_type = llvm::cast<llvm::StructType>(llvm_flexi_type(output_smooth_probe));
	destroy_dummy_igc(dummy2);

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
		new_value_table,
		igc.puts_func,
		igc.log_data_func,
		igc.log_data_deref_func,
		nullptr,
		igc.toc,
	};

	llvm::Argument* single_argument = func->getArg(0);
	single_argument->setName("InputMap");

	llvm::Value* input_alloca = func_builder.CreateAlloca(input_struct_type, nullptr, "input_struct");
	func_builder.CreateStore(single_argument, input_alloca);

	// if (!map->call_input_identifier.has_value()) {
	// 	fprintf(stderr, "must take in input using entire destructure for now.\n");
	// 	exit(1);
	// }

	if (map->call_input_identifier.has_value()) {
		std::string input_var_name = "m_" + map->call_input_identifier.value();

		ValueSymbolTableEntry input_entry{
			input_alloca,
			input_struct_type,
			Type(map->call_input_type),
			false,
			std::nullopt,
		};

		new_value_table->set(input_var_name, input_entry);
	}

	Smooth output_smooth = evaluate_smooth(enhanced_igc, Type(map->call_output_type));
	func_builder.CreateRet(llvm_value(output_smooth));

	(*p_bundle_map)->call_func = func;
	(*p_bundle_map)->call_func_alwaysinline = nullptr;

	return func;
}
