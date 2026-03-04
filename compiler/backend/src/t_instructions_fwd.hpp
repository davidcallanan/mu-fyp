#pragma once

#include <memory>
#include <variant>

struct InstructionSym;
struct InstructionExpr;
struct InstructionFor;
struct InstructionIf;

// Forward declaration impossible without pointer indirection, typical C++.

using Instruction = std::variant<
	std::shared_ptr<InstructionSym>,
	std::shared_ptr<InstructionExpr>,
	std::shared_ptr<InstructionFor>,
	std::shared_ptr<InstructionIf>
>;
