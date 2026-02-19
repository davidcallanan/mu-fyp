#pragma once

#include "dependencies/json.hpp"
#include "create_type_symbol_table.hpp"
#include "t_types.hpp"

using json = nlohmann::json;

Type normalize_type(
	const json& typeval,
	TypeSymbolTable& symbol_table
);
