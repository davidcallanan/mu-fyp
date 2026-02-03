#pragma once

#include <memory>
#include <variant>

struct TypeMap;

// Forward declaration impossible without pointer indirection, typical C++.

using Type = std::variant<
	std::shared_ptr<TypeMap>
>;
