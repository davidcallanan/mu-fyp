#include <cstdio>
#include <cstdlib>
#include <memory>
#include <variant>
#include "get_underlying_type.hpp"

const Type& get_underlying_type(const Type& type) {
	if (std::holds_alternative<std::shared_ptr<TypeMap>>(type)) {
		return type;
	}
	
	if (std::holds_alternative<std::shared_ptr<TypePointer>>(type)) {
		return type;
	}

	if (auto p_var_access = std::get_if<std::shared_ptr<TypeVarAccess>>(&type)) {
		const auto& var_access = *p_var_access;
		
		if (var_access->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated.\n");
			exit(1);
		}
		
		return get_underlying_type(*var_access->underlying_type);
	}
	
	fprintf(stderr, "Currently no mechanism to determine the actual type of the expression.\n");
	exit(1);
}
