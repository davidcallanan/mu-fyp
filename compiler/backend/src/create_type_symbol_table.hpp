#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include "t_types.hpp"
#include "create_bundle_registry.hpp"
#include "dependencies/json.hpp"

using json = nlohmann::json;

class TypeSymbolTable {
private:
	struct MapEntry {
		std::map<std::string, std::unique_ptr<MapEntry>> children;
		std::optional<UnderlyingType> type_value;
	};
	
	std::unique_ptr<MapEntry> _root;
	BundleRegistry* _bundle_registry;

	std::vector<std::string> _split_trail(const std::string& trail);

public:
	TypeSymbolTable(BundleRegistry& bundle_registry);
	
	void set(const std::string& trail, const UnderlyingType& value);
	
	std::optional<UnderlyingType> get(const std::string& trail);
};

TypeSymbolTable create_type_symbol_table(BundleRegistry& bundle_registry);
