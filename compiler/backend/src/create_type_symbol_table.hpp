#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>
#include "t_types.hpp"
#include "dependencies/json.hpp"

using json = nlohmann::json;

class TypeSymbolTable {
private:
	struct MapEntry {
		std::map<std::string, std::unique_ptr<MapEntry>> children;
		std::unique_ptr<TypeMap> type_value;
	};
	
	std::unique_ptr<MapEntry> _root;

	std::vector<std::string> _split_trail(const std::string& trail);

public:
	TypeSymbolTable();

	// decision for TypeMap is because it is working with an underlying type.
	// perhaps at some point there could be other underlying types.
	// pointers can always be wrapped, so for now i don't think so.
	
	void set(const std::string& trail, const TypeMap& value);
	
	TypeMap* get(const std::string& trail);
};

TypeSymbolTable create_type_symbol_table();
