#pragma once

#include <memory>
#include <variant>

struct TypeMap;
struct TypePointer;
struct TypeVarAccess;
struct TypeMerged;
struct TypeRotten;

// Forward declaration impossible without pointer indirection, typical C++.

using Type = std::variant<
	std::shared_ptr<TypeMap>,
	std::shared_ptr<TypePointer>,
	std::shared_ptr<TypeVarAccess>,
	std::shared_ptr<TypeMerged>,
	std::shared_ptr<TypeRotten>
>;
