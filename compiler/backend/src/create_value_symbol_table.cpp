#include "create_value_symbol_table.hpp"
#include <optional>

ValueSymbolTable::ValueSymbolTable() {
}

void ValueSymbolTable::set(const std::string& name, const ValueSymbolTableEntry& entry) {
	_values[name] = entry;
}

std::optional<ValueSymbolTableEntry> ValueSymbolTable::get(const std::string& name) {
	auto it = _values.find(name);
	
	if (it == _values.end()) {
		return std::nullopt;
	}
	
	return it->second;
}

ValueSymbolTable create_value_symbol_table() {
	return ValueSymbolTable();
}
