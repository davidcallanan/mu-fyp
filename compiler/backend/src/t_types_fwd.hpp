#pragma once

#include <memory>
#include <variant>

// While these are called types, they can often be thought of as expressions (as the design has shifted over time).

// The following terms are used interchangebly throughout this project, due to legacy naming:
// - expression
// - type
// - typeval

struct TypeMap;
struct TypePointer;
struct TypeVarAccess;
struct TypeMerged;
struct TypeRotten;
struct TypeLog;
struct TypeAssign;

// Forward declaration impossible without pointer indirection, typical C++.

using Type = std::variant<
	std::shared_ptr<TypeMap>,
	std::shared_ptr<TypePointer>,
	std::shared_ptr<TypeVarAccess>,
	std::shared_ptr<TypeMerged>,
	std::shared_ptr<TypeRotten>,
	std::shared_ptr<TypeLog>,
	std::shared_ptr<TypeAssign>
>;
