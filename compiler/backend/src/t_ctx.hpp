#pragma once

#include <memory>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "create_type_symbol_table.hpp"
#include "create_value_symbol_table.hpp"
#include "create_bundle_registry.hpp"

struct TypeOrchCtx {
	std::shared_ptr<TypeSymbolTable> type_table;
	std::shared_ptr<BundleRegistry> bundle_registry;
};

struct IrGenCtx {
	std::shared_ptr<llvm::LLVMContext> context;
	std::shared_ptr<llvm::Module> module;
	std::shared_ptr<llvm::IRBuilder<>> builder;
	std::shared_ptr<ValueSymbolTable> value_table;
	std::shared_ptr<llvm::FunctionCallee> puts_func;
	llvm::Function* log_data_func;
	llvm::Function* log_data_deref_func;
	llvm::BasicBlock* block_break;
	std::shared_ptr<TypeOrchCtx> toc;
};
