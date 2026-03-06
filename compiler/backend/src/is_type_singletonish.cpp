#include "is_type_singletonish.hpp"
#include "get_underlying_type.hpp"
#include "t_types.hpp"

bool is_type_singletonish(const Type& type) { // is there only one runtime value that the type encodes? if so, can be made static.
	Type underlying = get_underlying_type(type);

	if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&underlying)) {
		// previously hardval determined whether pointer was singletonish, however this is wrong.
		// hardval means the underlying data is singletonish, but this has nothing to do with the pointer itself, which may be mutated (e.g. if in a variable).
		// we kinda just assume pointers can take on any value, and if it genuinely has constant data,
		// llvm would be good enough at optimizing this (probably fairly common optimization).
		
		return false;
	}

	if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&underlying)) {
		return (*p_v_enum)->hardsym.has_value();
	}

	if (auto p_v_void = std::get_if<std::shared_ptr<TypeVoid>>(&underlying)) {
		return true;
	}

	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying)) {
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
