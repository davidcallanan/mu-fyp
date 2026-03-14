#include <cstdio>
#include <cstdlib>
#include <memory>
#include <variant>
#include "get_underlying_type.hpp"
#include "t_types.hpp"
#include "preinstantiated_types.hpp"
#include "merge_underlying_type.hpp"

// there are three scenarios:
// a. the node is already the most underlying type: return the same
// b. the node tracks underlying type information via the previously assigned .underlying_type field.
// c. the node doesn't really need type information: return TypeVoid.

Type get_underlying_type(const Type& type) {
	if (std::holds_alternative<std::shared_ptr<TypeRotten>>(type)) {
		return type;
	}

	if (std::holds_alternative<std::shared_ptr<TypeMap>>(type)) {
		return type;
	}
	if (std::holds_alternative<std::shared_ptr<TypePointer>>(type)) {
		return type;
	}

	if (std::holds_alternative<std::shared_ptr<TypeEnum>>(type)) {
		return type;
	}

	if (auto p_v_merged = std::get_if<std::shared_ptr<TypeMerged>>(&type)) {
		const auto& v_merged = *p_v_merged;

		if (v_merged->underlying_type == nullptr) {
			if (v_merged->types.size() != 2) {
				fprintf(stderr, "Underlying type not populatable (for TypeMerged).\n");
				exit(1);
			}

			Type result = merge_underlying_type(v_merged->types[0], v_merged->types[1]);
			
			v_merged->underlying_type = std::make_shared<Type>(result);
		}

		return get_underlying_type(*v_merged->underlying_type);
	}
	
	if (auto p_v_extern_ccc = std::get_if<std::shared_ptr<TypeExternCcc>>(&type)) {
		const auto& v_extern_ccc = *p_v_extern_ccc;
		
		if (v_extern_ccc->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExternCcc).\n");
			exit(1);
		}
		
		return get_underlying_type(*v_extern_ccc->underlying_type);
	}

	if (std::holds_alternative<std::shared_ptr<TypeVoid>>(type)) {
		return type;
	}

	if (std::holds_alternative<std::shared_ptr<TypeLog>>(type)) {
		return type_void;
	}

	if (std::holds_alternative<std::shared_ptr<TypeLogD>>(type)) {
		return type_void;
	}

	if (std::holds_alternative<std::shared_ptr<TypeLogDd>>(type)) {
		return type_void;
	}
	
	if (std::holds_alternative<std::shared_ptr<TypeExternCcc>>(type)) {
		return type;
	}

	if (auto p_v_call_with_sym = std::get_if<std::shared_ptr<TypeCallWithSym>>(&type)) {
		const auto& v_call_with_sym = *p_v_call_with_sym;

		if (v_call_with_sym->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeCallWithSym).\n");
			exit(1);
		}

		return get_underlying_type(*v_call_with_sym->underlying_type);
	}

	if (auto p_v_call_with_dynamic = std::get_if<std::shared_ptr<TypeCallWithDynamic>>(&type)) {
		const auto& v_call_with_dynamic = *p_v_call_with_dynamic;

		if (v_call_with_dynamic->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeCallWithDynamic).\n");
			exit(1);
		}

		return get_underlying_type(*v_call_with_dynamic->underlying_type);
	}

	if (auto p_v_expr_multi = std::get_if<std::shared_ptr<TypeExprMulti>>(&type)) {
		const auto& v_expr_multi = *p_v_expr_multi;

		if (v_expr_multi->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprMulti).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_multi->underlying_type);
	}

	if (auto p_v_expr_addit = std::get_if<std::shared_ptr<TypeExprAddit>>(&type)) {
		const auto& v_expr_addit = *p_v_expr_addit;

		if (v_expr_addit->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprAddit).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_addit->underlying_type);
	}

	if (auto p_v_expr_logical_and = std::get_if<std::shared_ptr<TypeExprLogicalAnd>>(&type)) {
		const auto& v_expr_logical_and = *p_v_expr_logical_and;

		if (v_expr_logical_and->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprLogicalAnd).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_logical_and->underlying_type);
	}

	if (auto p_v_expr_logical_or = std::get_if<std::shared_ptr<TypeExprLogicalOr>>(&type)) {
		const auto& v_expr_logical_or = *p_v_expr_logical_or;

		if (v_expr_logical_or->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprLogicalOr).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_logical_or->underlying_type);
	}

	if (auto p_v_var_access = std::get_if<std::shared_ptr<TypeVarAccess>>(&type)) {
		const auto& v_var_access = *p_v_var_access;
		
		if (v_var_access->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeVarAccess): %s.\n", v_var_access->target_name.c_str());
			exit(1);
		}
		
		return get_underlying_type(*v_var_access->underlying_type);
	}
	
	if (auto p_v_walrus = std::get_if<std::shared_ptr<TypeVarWalrus>>(&type)) {
		const auto& v_walrus = *p_v_walrus;
		
		if (v_walrus->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeVarWalrus): %s.\n", v_walrus->name.c_str());
			exit(1);
		}
		
		return get_underlying_type(*v_walrus->underlying_type);
	}
	
	if (auto p_v_var_assign = std::get_if<std::shared_ptr<TypeVarAssign>>(&type)) {
		const auto& v_var_assign = *p_v_var_assign;
		
		if (v_var_assign->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeVarAssign): %s.\n", v_var_assign->name.c_str());
			exit(1);
		}
		
		return get_underlying_type(*v_var_assign->underlying_type);
	}
	
	if (std::holds_alternative<std::shared_ptr<TypeMapReference>>(type)) {
		return type;
	}

	if (auto p_v_take_address = std::get_if<std::shared_ptr<TypeTakeAddress>>(&type)) {
		const auto& v_take_address = *p_v_take_address;

		if (v_take_address->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeTakeAddress).\n");
			exit(1);
		}

		return get_underlying_type(*v_take_address->underlying_type);
	}
	
	if (auto p_v_sizeof = std::get_if<std::shared_ptr<TypeSizeof>>(&type)) {
		const auto& v_sizeof = *p_v_sizeof;

		if (v_sizeof->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeSizeof).\n");
			exit(1);
		}

		return get_underlying_type(*v_sizeof->underlying_type);
	}
	
	if (auto p_v_expr_not = std::get_if<std::shared_ptr<TypeExprNot>>(&type)) {
		const auto& v_expr_not = *p_v_expr_not;

		if (v_expr_not->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprNot).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_not->underlying_type);
	}

	if (auto p_v_expr_shift_right = std::get_if<std::shared_ptr<TypeExprShiftRight>>(&type)) {
		const auto& v_expr_shift_right = *p_v_expr_shift_right;

		if (v_expr_shift_right->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprShiftRight).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_shift_right->underlying_type);
	}

	if (auto p_v_expr_shift_left = std::get_if<std::shared_ptr<TypeExprShiftLeft>>(&type)) {
		const auto& v_expr_shift_left = *p_v_expr_shift_left;

		if (v_expr_shift_left->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprShiftLeft).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_shift_left->underlying_type);
	}

	if (auto p_v_expr_bitwise_and = std::get_if<std::shared_ptr<TypeExprBitwiseAnd>>(&type)) {
		const auto& v_expr_bitwise_and = *p_v_expr_bitwise_and;

		if (v_expr_bitwise_and->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprBitwiseAnd).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_bitwise_and->underlying_type);
	}

	if (auto p_v_expr_bitwise_or = std::get_if<std::shared_ptr<TypeExprBitwiseOr>>(&type)) {
		const auto& v_expr_bitwise_or = *p_v_expr_bitwise_or;

		if (v_expr_bitwise_or->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprBitwiseOr).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_bitwise_or->underlying_type);
	}

	if (auto p_v_expr_modulo = std::get_if<std::shared_ptr<TypeExprModulo>>(&type)) {
		const auto& v_expr_modulo = *p_v_expr_modulo;

		if (v_expr_modulo->underlying_type == nullptr) {
			fprintf(stderr, "Underlying type not populated (TypeExprModulo).\n");
			exit(1);
		}

		return get_underlying_type(*v_expr_modulo->underlying_type);
	}
	
	if (std::holds_alternative<std::shared_ptr<TypeNullptr>>(type)) {
		return type_voidptr;
	}

	if (std::holds_alternative<std::shared_ptr<TypeVoidptr>>(type)) {
		return type_voidptr;
	}
	
	if (std::holds_alternative<std::shared_ptr<TypeVoidMapReference>>(type)) {
		return type;
	}

	fprintf(stderr, "Currently no mechanism to determine the actual type of the expression.\n");
	exit(1);
}
