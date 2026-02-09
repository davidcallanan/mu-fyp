#pragma once

#include <map>
#include <string>
#include <optional>
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "t_types_fwd.hpp"

struct ValueSymbolTableEntry {
	llvm::Value* alloca_ptr;
	llvm::Type* ir_type;
	Type type;
	bool has_leaf;
};

class ValueSymbolTable {
private:
	std::map<std::string, ValueSymbolTableEntry> _values;

public:
	ValueSymbolTable();

	void set(const std::string& name, const ValueSymbolTableEntry& entry);

	std::optional<ValueSymbolTableEntry> get(const std::string& name);
};

ValueSymbolTable create_value_symbol_table();
