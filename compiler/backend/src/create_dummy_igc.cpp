#include "create_value_symbol_table.hpp"

#include <memory>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "create_dummy_igc.hpp"

DummyIgc create_dummy_igc(std::shared_ptr<IrGenCtx> base) {
	llvm::FunctionType* dummy_func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(*base->context), false);
	llvm::Function* dummy_func = llvm::Function::Create(dummy_func_type, llvm::Function::PrivateLinkage, "__ec_dummy", *base->module);
	llvm::BasicBlock* dummy_block = llvm::BasicBlock::Create(*base->context, "entry", dummy_func);

	auto dummy_builder = std::make_shared<llvm::IRBuilder<>>(*base->context);
	dummy_builder->SetInsertPoint(dummy_block);

	auto igc = std::make_shared<IrGenCtx>(IrGenCtx{
		base->context,
		base->module,
		dummy_builder,
		std::make_shared<ValueSymbolTable>(create_value_symbol_table()),
		base->puts_func,
		base->log_data_func,
		base->log_data_deref_func,
		nullptr,
		base->toc,
	});

	return DummyIgc{
		igc,
		dummy_builder,
		dummy_func,
	};
}
