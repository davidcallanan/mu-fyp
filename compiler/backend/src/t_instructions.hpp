#pragma once

#include <string>
#include <variant>

struct InstructionLog {
	std::string message;
};

using Instruction = std::variant<
	InstructionLog
>;
