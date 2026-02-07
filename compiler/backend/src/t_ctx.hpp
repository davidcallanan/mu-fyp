#pragma once

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "create_type_symbol_table.hpp"
#include "create_value_symbol_table.hpp"

struct IrGenCtx {
	llvm::LLVMContext& context;
	llvm::Module& module;
	llvm::IRBuilder<>& builder;
	TypeSymbolTable& type_table;
	ValueSymbolTable& value_table;
	llvm::FunctionCallee& puts_func;
};
