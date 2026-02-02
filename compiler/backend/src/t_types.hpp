#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include "t_instructions.hpp"

struct TypeMap;

using Type = std::variant<
	TypeMap
>;

struct TypeMap {
	std::string leaf_type;
	std::shared_ptr<TypeMap> call_input_type;
	std::shared_ptr<TypeMap> call_output_type;
	std::map<std::string, std::shared_ptr<Type>> sym_inputs;
	std::vector<Instruction> execution_sequence;
};
