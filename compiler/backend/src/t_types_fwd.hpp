#pragma once

#include <memory>
#include <variant>

// While these are called types, they can often be thought of as expressions (as the design has shifted over time).

// The following terms are used fairly interchangebly throughout this project, due to legacy naming:
// - expression
// - type
// - typeval
// - constraint

struct TypeMap;
struct TypePointer;
struct TypeVarAccess;
struct TypeMerged;
struct TypeRotten;
struct TypeLog;
struct TypeVarWalrus;
struct TypeVarAssign;
struct TypeCallWithSym;
struct TypeExprMulti;
struct TypeExprAddit;

// Forward declaration impossible without pointer indirection, typical C++.

using Type = std::variant<
	std::shared_ptr<TypeMap>,
	std::shared_ptr<TypePointer>,
	std::shared_ptr<TypeVarAccess>,
	std::shared_ptr<TypeMerged>,
	std::shared_ptr<TypeRotten>,
	std::shared_ptr<TypeLog>,
	std::shared_ptr<TypeVarWalrus>,
	std::shared_ptr<TypeVarAssign>,
	std::shared_ptr<TypeCallWithSym>,
	std::shared_ptr<TypeExprMulti>,
	std::shared_ptr<TypeExprAddit>
>;

using UnderlyingType = std::variant< // an underlying type is kind of the eventual type after evaluation.
	std::shared_ptr<TypeMap>,
	std::shared_ptr<TypePointer>,
	std::shared_ptr<TypeMerged>,
	std::shared_ptr<TypeRotten>
>;
