#include "create_value_symbol_table.hpp"
#include <optional>

ValueSymbolTable::ValueSymbolTable() {
}

void ValueSymbolTable::set(const std::string& name, llvm::Value* value) {
	_values[name] = value;
}

std::optional<llvm::Value*> ValueSymbolTable::get(const std::string& name) {
	auto it = _values.find(name);
	
	if (it == _values.end()) {
		return std::nullopt;
	}
	
	return it->second;
}

ValueSymbolTable create_value_symbol_table() {
	return ValueSymbolTable();
}
