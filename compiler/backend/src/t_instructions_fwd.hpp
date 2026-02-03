#pragma once

#include <memory>
#include <variant>

struct InstructionLog;
struct InstructionAssign;

// Forward declaration impossible without pointer indirection, typical C++.

using Instruction = std::variant<
	std::shared_ptr<InstructionLog>,
	std::shared_ptr<InstructionAssign>
>;
