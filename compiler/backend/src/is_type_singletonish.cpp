#include "is_type_singletonish.hpp"
#include "t_types.hpp"

bool is_type_singletonish(const Type& type) { // is there only one runtime value that the type encodes? if so, can be made static.
	if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
		return (*p_v_pointer)->hardval.has_value();
	}

	if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&type)) {
		return (*p_v_enum)->hardsym.has_value();
	}

	if (auto p_v_void = std::get_if<std::shared_ptr<TypeVoid>>(&type)) {
		return true;
	}

	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&type)) {
		return false; // unnecessary optimization, as llvm should optimize an empty struct automatically.		
		
		const TypeMap& map = **p_v_map;

		if (map.leaf_type.has_value()) {
			if (!is_type_singletonish(map.leaf_type.value())) {
				return false;
			}
		}

		for (const auto& [_sym_name, sym_type_ptr] : map.sym_inputs) {
			if (!is_type_singletonish(*sym_type_ptr)) {
				return false;
			}
		}

		return true;
	}

	return false;
}
