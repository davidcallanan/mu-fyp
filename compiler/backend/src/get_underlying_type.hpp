#pragma once

#include "t_types.hpp"
#include "create_value_symbol_table.hpp"

Type get_underlying_type(const Type& type, ValueSymbolTable* value_table);
