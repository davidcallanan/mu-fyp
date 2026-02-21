#pragma once

#include <string>
#include <memory>
#include "t_instructions_fwd.hpp"
#include "t_types_fwd.hpp"

struct InstructionSym {
	std::string name;
	std::shared_ptr<Type> typeval;
};

struct InstructionExpr {
	std::shared_ptr<Type> expr;
};

struct InstructionFor {
	std::shared_ptr<TypeMap> body;
};

