#pragma once

#include <memory>
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "t_ctx.hpp"
#include "create_value_symbol_table.hpp"

struct DummyIgc {
	std::shared_ptr<IrGenCtx> igc;
	std::shared_ptr<llvm::IRBuilder<>> dummy_builder;
	llvm::Function* dummy_func;
};

DummyIgc create_dummy_igc(std::shared_ptr<IrGenCtx> base);
