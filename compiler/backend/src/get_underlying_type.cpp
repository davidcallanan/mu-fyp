#include <cstdio>
#include <cstdlib>
#include <memory>
#include <variant>
#include "get_underlying_type.hpp"

Type get_underlying_type(const Type& type) {
	if (std::holds_alternative<std::shared_ptr<TypeMap>>(type)) {
		return type;
	}
	
	if (std::holds_alternative<std::shared_ptr<TypePointer>>(type)) {
		return type;
	}

	if (auto p_v_var_access = std::get_if<std::shared_ptr<TypeVarAccess>>(&type)) {
		const auto& v_var_access = *p_v_var_access;
		
		if (v_var_access->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (VarAccess): %s.\n", v_var_access->target_name.c_str());
			exit(1);
		}
		
		return get_underlying_type(*v_var_access->underlying_type);
	}
	
	if (auto p_v_walrus = std::get_if<std::shared_ptr<TypeVarWalrus>>(&type)) {
		const auto& v_walrus = *p_v_walrus;
		
		if (v_walrus->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (VarWalrus): %s.\n", v_walrus->name.c_str());
			exit(1);
		}
		
		return get_underlying_type(*v_walrus->underlying_type);
	}
	
	if (auto p_v_var_assign = std::get_if<std::shared_ptr<TypeVarAssign>>(&type)) {
		const auto& v_var_assign = *p_v_var_assign;
		
		if (v_var_assign->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (VarAssign): %s.\n", v_var_assign->name.c_str());
			exit(1);
		}
		
		return get_underlying_type(*v_var_assign->underlying_type);
	}
	
	fprintf(stderr, "Currently no mechanism to determine the actual type of the expression.\n");
	exit(1);
}
