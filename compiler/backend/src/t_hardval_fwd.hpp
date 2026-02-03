#pragma once

#include <memory>
#include <variant>

struct HardvalInteger;
struct HardvalFloat;

// Forward declaration impossible without pointer indirection, typical C++.

using Hardval = std::variant<
	std::shared_ptr<HardvalInteger>,
	std::shared_ptr<HardvalFloat>
>;
