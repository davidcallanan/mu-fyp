#pragma once

#include <memory>
#include <variant>

struct InstructionLog;
struct InstructionAssign;
struct InstructionSym;

// Forward declaration impossible without pointer indirection, typical C++.

using Instruction = std::variant<
	std::shared_ptr<InstructionLog>,
	std::shared_ptr<InstructionAssign>,
	std::shared_ptr<InstructionSym>
>;
