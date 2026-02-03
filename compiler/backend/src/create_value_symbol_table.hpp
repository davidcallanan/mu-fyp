#pragma once

#include <map>
#include <string>
#include <optional>
#include "llvm/IR/Value.h"

class ValueSymbolTable {
private:
	std::map<std::string, llvm::Value*> _values;

public:
	ValueSymbolTable();

	void set(const std::string& name, llvm::Value* value);

	std::optional<llvm::Value*> get(const std::string& name);
};

ValueSymbolTable create_value_symbol_table();
