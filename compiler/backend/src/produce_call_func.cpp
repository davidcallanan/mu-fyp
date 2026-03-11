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
#include "fresh_smooth.hpp"
#include "clone_type_map_for_mutation.hpp"

llvm::Function* produce_call_func(
	std::shared_ptr<IrGenCtx> igc,
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
	
	Bundle* bundle = igc->toc->bundle_registry->get(map->bundle_id.value());
	
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
	
	if ((*p_bundle_map)->reentrancy_prevention) {
		fprintf(stderr, "Re-entrancy invariant violated! This is a serious bug due to circular initialization logic. Can you run produce_call_func later? Do you really need to results now?\n");
		exit(1);
	}

	(*p_bundle_map)->reentrancy_prevention = true;

	if (is_alwaysinline) {
		return nullptr;
	}

	DummyIgc dummy1 = create_dummy_igc(igc);
	auto input_shell = clone_type_map_for_mutation(igc, map->call_input_type);
	// we cannot clear, we need to process the syms to know what types they are.
	// input_shell->execution_sequence.clear();
	Smooth input_smooth = evaluate_smooth(dummy1.igc, Type(input_shell));
	llvm::StructType* input_struct_type = llvm::cast<llvm::StructType>(llvm_flexi_type(input_smooth));
	destroy_dummy_igc(dummy1);

	DummyIgc dummy2 = create_dummy_igc(igc);
	auto v_map = clone_type_map_for_mutation(igc, map->call_output_type);
	// we have to clear, because the body only makes sense to execute in the context of where it is called.
	// for now it is impossible to determine type information.
	// but all our internal call funcs are void for now, so less relevant.
	// i have no idea how i would fix this.
	// like somehow pass in types instead of llvm::Value's, it is impossible in the current system.
	// literally could be a 5k+ loc change.
	v_map->execution_sequence.clear();
	Smooth output_smooth_probe = evaluate_smooth(dummy2.igc, Type(v_map));
	llvm::StructType* output_struct_type = llvm::cast<llvm::StructType>(llvm_flexi_type(output_smooth_probe));
	destroy_dummy_igc(dummy2);

	// ah! llvm has moved to opaque pointers for everything, so we use everywhere!
	llvm::Type* opaque_pointer = llvm::PointerType::get(*igc->context, 0);

	llvm::FunctionType* func_type = llvm::FunctionType::get(
		output_struct_type,
		{ opaque_pointer, opaque_pointer, input_struct_type },
		false
	);

	static int next_callable_id = 0;
	std::string func_name = "__ec_callable_" + std::to_string(next_callable_id++);

	llvm::Function* func = llvm::Function::Create(
		func_type,
		llvm::Function::PrivateLinkage,
		func_name,
		*igc->module
	);

	llvm::BasicBlock* func_entry = llvm::BasicBlock::Create(*igc->context, "entry", func);
	auto func_builder = std::make_shared<llvm::IRBuilder<>>(*igc->context);
	func_builder->SetInsertPoint(func_entry);

	std::shared_ptr<ValueSymbolTable> new_value_table = std::make_shared<ValueSymbolTable>( // currently no closure ability implemented.
		create_value_symbol_table()
	);

	auto enhanced_igc = std::make_shared<IrGenCtx>(IrGenCtx{
		igc->context,
		igc->module,
		func_builder,
		new_value_table,
		igc->puts_func,
		igc->log_data_func,
		igc->log_data_deref_func,
		nullptr,
		igc->toc,
	});

	llvm::Argument* arg_mod = func->getArg(0);
	llvm::Argument* arg_this = func->getArg(1);
	llvm::Argument* single_argument = func->getArg(2);
	arg_mod->setName("InputMod");
	arg_this->setName("InputThis");
	single_argument->setName("InputMap");

	llvm::Value* input_alloca = func_builder->CreateAlloca(input_struct_type, nullptr, "input_struct");
	func_builder->CreateStore(single_argument, input_alloca);

	// Here we merely re-expose mod.
	{
		std::optional<ValueSymbolTableEntry> o_entry_mod = igc->value_table->get("m_mod");

		if (o_entry_mod.has_value()) {
			llvm::Type* opaque_pointer = llvm::PointerType::get(*enhanced_igc->context, 0);
			
			llvm::Value* mod_alloca = func_builder->CreateAlloca(opaque_pointer, nullptr, "AllocaMod");
			func_builder->CreateStore(arg_mod, mod_alloca);
			
			ValueSymbolTableEntry entry_mod = o_entry_mod.value();
			entry_mod.alloca_ptr = mod_alloca;

			if (entry_mod.smooth.has_value()) {
				entry_mod.smooth = fresh_smooth(enhanced_igc, entry_mod.smooth.value(), arg_mod);
			}

			new_value_table->set("m_mod", entry_mod);
		}
	}

	// Here we have to populate "this" from the actual data.
	{
		auto p_v_map_reference = std::make_shared<TypeMapReference>();
		p_v_map_reference->target = map;

		Bundle* bundle = igc->toc->bundle_registry->get(map->bundle_id.value());
		
		if (!bundle) {
			fprintf(stderr, "Bundle missing.\n");
			exit(1);
		}
		
		auto p_bundle_map = std::get_if<std::shared_ptr<BundleMap>>(bundle);
		
		if (!p_bundle_map) {
			fprintf(stderr, "Bundle map missing.");
			exit(1);
		}
		
		auto bundle_map = *p_bundle_map;
		
		llvm::Type* this_llvm_type = bundle_map->opaque_struct_type;

		llvm::Type* opaque_pointer = llvm::PointerType::get(*enhanced_igc->context, 0);
		
		llvm::Value* this_alloca = func_builder->CreateAlloca(opaque_pointer, nullptr, "AllocaThis");
		func_builder->CreateStore(arg_this, this_alloca);

		new_value_table->set("m_this", ValueSymbolTableEntry{
			this_alloca,
			opaque_pointer,
			Type(p_v_map_reference),
			false,
			std::optional<Smooth>(std::make_shared<SmoothMapReference>(SmoothMapReference{
				Type(p_v_map_reference),
				arg_this,
				this_llvm_type,
			})),
		});
	}

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
	
	(*p_bundle_map)->call_func = func;
	(*p_bundle_map)->call_func_alwaysinline = nullptr;

	Smooth output_smooth = evaluate_smooth(enhanced_igc, Type(map->call_output_type));
	func_builder->CreateRet(llvm_value(output_smooth));

	return func;
}
