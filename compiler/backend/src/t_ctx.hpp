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
	TypeSymbolTable type_table;
	BundleRegistry bundle_registry;
};

struct IrGenCtx {
	llvm::LLVMContext& context;
	llvm::Module& module;
	llvm::IRBuilder<>& builder;
	std::shared_ptr<ValueSymbolTable> value_table;
	llvm::FunctionCallee& puts_func;
	llvm::Function* log_data_func;
	llvm::Function* log_data_deref_func;
	llvm::BasicBlock* block_break;
	std::shared_ptr<TypeOrchCtx> toc;
};
