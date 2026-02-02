#include "create_type_symbol_table.hpp"
#include <cstdio>
#include <cstdlib>

std::vector<std::string> TypeSymbolTable::_split_trail(const std::string &trail) {
	std::vector<std::string> segments;
	size_t start = 0;
	size_t end = trail.find("::");

	while (end != std::string::npos) {
		segments.push_back(trail.substr(start, end - start));
		start = end + 2;
		end = trail.find("::", start);
	}

	segments.push_back(trail.substr(start));

	return segments;
}

TypeSymbolTable::TypeSymbolTable() {
	_root = std::make_unique<MapEntry>();

	const std::vector<std::string> float_types = {"f16", "f32", "f64", "f128"};

	for (const auto &leaf_type : float_types) {
		auto map = std::make_unique<TypeMap>();
		map->leaf_type = leaf_type;
		map->call_input_type = nullptr;
		map->call_output_type = nullptr;

		auto entry = std::make_unique<MapEntry>();
		entry->type_value = std::move(map);
		_root->children[leaf_type] = std::move(entry);
	}
}

void TypeSymbolTable::set(const std::string &trail, const TypeMap &value) {
	auto segments = _split_trail(trail);

	if (segments.empty()) {
		fprintf(stderr, "Empty trail was provided for some reason\n");
		exit(1);
	}

	MapEntry *curr_entry = _root.get();

	for (const auto &segment : segments) {
		if (curr_entry->children.find(segment) == curr_entry->children.end()) {
			curr_entry->children[segment] = std::make_unique<MapEntry>();
		}

		curr_entry = curr_entry->children[segment].get();
	}

	curr_entry->type_value = std::make_unique<TypeMap>(value);
}

TypeMap *TypeSymbolTable::get(const std::string &trail) {
	auto segments = _split_trail(trail);

	if (segments.empty()) {
		fprintf(stderr, "Empty trail was provided for some reason\n");
		exit(1);
	}

	MapEntry *curr_entry = _root.get();

	for (size_t i = 0; i < segments.size(); i++) {
		const auto &segment = segments[i];

		if (curr_entry->children.find(segment) == curr_entry->children.end()) {
			if (i == segments.size() - 1) {
				break;
			}
			
			return nullptr;
		}

		curr_entry = curr_entry->children[segment].get();
	}

	const std::string &key = segments[segments.size() - 1];

	if (curr_entry->children.find(key) != curr_entry->children.end()) {
		return curr_entry->children[key]->type_value.get();
	}

	if (true
		&& segments.size() == 1
		&& key.length() >= 2
		&& (key[0] == 'i' || key[0] == 'u')
		&& key[1] >= '1'
		&& key[1] <= '9'
	) {
		bool is_valid_intish = true;

		for (size_t j = 2; j < key.length(); j++) {
			if (!isdigit(key[j])) {
				is_valid_intish = false;
				break;
			}
		}

		if (is_valid_intish) {
			auto map = std::make_unique<TypeMap>();
			map->leaf_type = key;
			map->call_input_type = nullptr;
			map->call_output_type = nullptr;

			auto entry = std::make_unique<MapEntry>();
			entry->type_value = std::move(map);

			TypeMap *result = entry->type_value.get();
			curr_entry->children[key] = std::move(entry);

			return result;
		}
	}

	return nullptr;
}

TypeSymbolTable create_type_symbol_table() {
	return TypeSymbolTable();
}
