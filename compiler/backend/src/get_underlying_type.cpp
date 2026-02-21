#include <cstdio>
#include <cstdlib>
#include <memory>
#include <variant>
#include "get_underlying_type.hpp"

Type get_underlying_type(const Type& type, ValueSymbolTable* value_table) {
	if (std::holds_alternative<std::shared_ptr<TypeMap>>(type)) {
		return type;
	}
	
	if (std::holds_alternative<std::shared_ptr<TypePointer>>(type)) {
		return type;
	}

	if (auto p_var_access = std::get_if<std::shared_ptr<TypeVarAccess>>(&type)) {
		const auto& var_access = *p_var_access;
		std::string var_name = "m_" + var_access->target_name;
		std::optional<ValueSymbolTableEntry> o_entry = value_table->get(var_name);
		
		if (!o_entry.has_value()) {
			fprintf(stderr, "Underlying type not populated.\n");
			fprintf(stderr, "Underlying failed to determine type information because consultation of symbol table failed on %s.\n", var_access->target_name.c_str());
			exit(1);
		}
		
		return get_underlying_type(o_entry->type, value_table);
	}
	
	fprintf(stderr, "Currently no mechanism to determine the actual type of the expression.\n");
	exit(1);
}
