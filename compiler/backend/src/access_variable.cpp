#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include "access_variable.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"
#include "create_value_symbol_table.hpp"
#include "llvm_to_smooth.hpp"

Smooth access_variable(
	IrGenCtx& igc,
	const Type& node
) {
	std::string target_name;
	std::shared_ptr<Type>* underlying_type = nullptr;
	
	if (auto p = std::get_if<std::shared_ptr<TypeVarAccess>>(&node)) {
		target_name = (*p)->target_name;
		underlying_type = &(*p)->underlying_type;
	} else if (auto p = std::get_if<std::shared_ptr<TypeVarWalrus>>(&node)) {
		target_name = (*p)->name;
		underlying_type = &(*p)->underlying_type;
	} else if (auto p = std::get_if<std::shared_ptr<TypeVarAssign>>(&node)) {
		target_name = (*p)->name;
		underlying_type = &(*p)->underlying_type;
	} else {
		fprintf(stderr, "only TypeVarWalrus, TypeVarAssign, and TypeVarAccess can access a variable!\n");
		exit(1);
	}
	
	std::string var_name = "m_" + target_name;
	std::optional<ValueSymbolTableEntry> o_entry = igc.value_table->get(var_name);
	
	if (!o_entry.has_value()) {
		std::string sym_var_name = "ms_:" + target_name;
		o_entry = igc.value_table->get(sym_var_name);
		
		if (!o_entry.has_value()) {
			fprintf(stderr, "This variable %s was not actually present in our value table\n", target_name.c_str());
			exit(1);
		}
	}
	
	const ValueSymbolTableEntry& entry = o_entry.value();
	
	*underlying_type = std::make_shared<Type>(entry.type);
	
	llvm::Value* loaded = igc.builder.CreateLoad(
		entry.ir_type,
		entry.alloca_ptr
	);

	return llvm_to_smooth(entry.type, loaded);
}
