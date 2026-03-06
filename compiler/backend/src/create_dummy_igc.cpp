#include "create_value_symbol_table.hpp"

#include <memory>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "create_dummy_igc.hpp"

DummyIgc create_dummy_igc(IrGenCtx& base) {
	llvm::FunctionType* dummy_func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(base.context), false);
	llvm::Function* dummy_func = llvm::Function::Create(dummy_func_type, llvm::Function::PrivateLinkage, "__ec_dummy", base.module);
	llvm::BasicBlock* dummy_block = llvm::BasicBlock::Create(base.context, "entry", dummy_func);

	auto dummy_builder = std::make_unique<llvm::IRBuilder<>>(base.context);
	dummy_builder->SetInsertPoint(dummy_block);

	IrGenCtx igc = {
		base.context,
		base.module,
		*dummy_builder,
		base.type_table,
		std::make_shared<ValueSymbolTable>(create_value_symbol_table()),
		base.puts_func,
		base.log_data_func,
		base.log_data_deref_func,
		nullptr,
	};

	return DummyIgc{
		igc,
		std::move(dummy_builder),
		dummy_func,
	};
}
