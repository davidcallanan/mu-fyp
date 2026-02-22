#pragma once

#include "dependencies/json.hpp"
#include "create_type_symbol_table.hpp"
#include "t_types.hpp"

using json = nlohmann::json;

// This function both parses and normalizes the JSON into a nice programmatic structure.

Type normalize_type(
	const json& typeval,
	TypeSymbolTable& symbol_table
);
