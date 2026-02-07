#pragma once

#include <memory>
#include <variant>

struct TypeMap;
struct TypePointer;

// Forward declaration impossible without pointer indirection, typical C++.

using Type = std::variant<
	std::shared_ptr<TypeMap>,
	std::shared_ptr<TypePointer>
>;
