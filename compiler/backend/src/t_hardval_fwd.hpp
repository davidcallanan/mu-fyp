#pragma once

#include <memory>
#include <variant>

struct HardvalInteger;
struct HardvalFloat;
struct HardvalString;
struct HardvalVarAccess;

// Forward declaration impossible without pointer indirection, typical C++.

using Hardval = std::variant<
	std::shared_ptr<HardvalInteger>,
	std::shared_ptr<HardvalFloat>,
	std::shared_ptr<HardvalString>,
	std::shared_ptr<HardvalVarAccess>
>;
