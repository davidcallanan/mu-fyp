#pragma once

#include <string>
#include <variant>
#include <memory>
#include "t_instructions_fwd.hpp"
#include "t_types_fwd.hpp"

struct InstructionLog {
	std::shared_ptr<Type> message;
};

struct InstructionAssign {
	std::string name;
	std::shared_ptr<Type> typeval;
};

