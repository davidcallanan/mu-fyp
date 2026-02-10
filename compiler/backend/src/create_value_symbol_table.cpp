#include "create_value_symbol_table.hpp"
#include <optional>

static std::string to_scope_id(int value) {
	const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const int alphabet_size = static_cast<int>(alphabet.size());
	const int scope_length = 6;
	std::string scope_id(scope_length, 'a');
	int remaining = value;
	
	for (int i = scope_length - 1; i >= 0; i--) {
		int digit = remaining % alphabet_size;
		remaining = remaining / alphabet_size;
		scope_id[i] = alphabet[static_cast<size_t>(digit)];
	}
	
	return scope_id;
}

static std::string next_scope_id() {
	static int scope_id = 0;
	return to_scope_id(scope_id++);
}

ValueSymbolTable::ValueSymbolTable(ValueSymbolTable* parent)
	: _parent(parent),
	_scope_id(next_scope_id()) {
}

void ValueSymbolTable::set(const std::string& name, const ValueSymbolTableEntry& entry) {
	_values[name] = entry;
}

std::optional<ValueSymbolTableEntry> ValueSymbolTable::get(const std::string& name) {
	auto it = _values.find(name);
	
	if (it == _values.end()) {
		if (_parent == nullptr) {
			return std::nullopt;
		}
		
		return _parent->get(name);
	}
	
	return it->second;
}

const std::string& ValueSymbolTable::scope_id() const {
	return _scope_id;
}

ValueSymbolTable create_value_symbol_table() {
	return ValueSymbolTable(nullptr);
}

ValueSymbolTable create_value_symbol_table(ValueSymbolTable* parent) {
	return ValueSymbolTable(parent);
}
