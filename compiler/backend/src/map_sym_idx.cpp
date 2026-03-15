#include "map_sym_idx.hpp"

#include "t_types.hpp"
#include "is_type_singletonish.hpp"

std::optional<size_t> map_sym_idx(std::shared_ptr<TypeMap> map, const std::string& sym_name) {
	auto it = map->sym_inputs.find(sym_name);
	if (it == map->sym_inputs.end()) {
		return std::nullopt;
	}

	if (is_type_singletonish(*it->second)) {
		return std::nullopt;
	}

	size_t idx = (false
		|| !map->leaf_type.has_value()
		|| is_type_singletonish(map->leaf_type.value())
	) ? 0 : 1;

	for (const auto& [name, type] : map->sym_inputs) {
		if (name == sym_name) {
			return idx;
		}
		
		if (!is_type_singletonish(*type)) {
			idx++;
		}
	}

	return std::nullopt;
}
