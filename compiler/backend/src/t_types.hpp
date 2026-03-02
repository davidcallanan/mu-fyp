#pragma once

#include <memory>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <variant>
#include "t_types_fwd.hpp"
#include "t_instructions_fwd.hpp"
#include "t_hardval_fwd.hpp"

struct TypeMap {
	std::optional<Type> leaf_type;
	std::optional<Hardval> leaf_hardval;
	std::shared_ptr<TypeMap> call_input_type;
	std::shared_ptr<TypeMap> call_output_type;
	std::map<std::string, std::shared_ptr<Type>> sym_inputs;
	std::vector<Instruction> execution_sequence;
};

struct TypePointer {
	std::shared_ptr<Type> target;
	std::optional<Hardval> hardval; // eventually this may need to be organized better, not sure how to best approach it.
};

struct TypeVarAccess {
	std::string target_name;
	std::shared_ptr<Type> underlying_type;
};

struct TypeMerged {
	std::vector<Type> types;
	std::shared_ptr<Type> underlying_type;
};

struct TypeRotten {
	std::string type_str;
};

struct TypeEnum {
	bool is_instantiated; // to distinguish between no hardsym vs unknown (runtime) hardsym.
	// wait, i should wrap in an overlying type instead of using an is_instantiated
	std::optional<std::string> hardsym; // this is a restrictive constraint, when merging, we imagine any value that satisfies ALL.
	std::vector<std::string> syms; // this is a permissive constraint, when merging, imagine any value that satisfies ANY.
};

struct TypeVoid {
};

struct TypeLog {
	std::shared_ptr<Type> message;
};

struct TypeLogD {
	std::shared_ptr<Type> message;
};

struct TypeLogDd {
	std::shared_ptr<Type> message;
	std::shared_ptr<Type> byte_count;
	bool is_nullterm;
};

struct TypeVarWalrus {
	std::string name;
	bool is_mut;
	std::shared_ptr<Type> typeval;
	std::shared_ptr<Type> underlying_type; // note this is not the underlying type of the assignment, but rather the usage of assignment as an expression, which evaluates to a variable access - equivalent to underlying_type in TypeVarAccess.
};

struct TypeVarAssign {
	std::string name;
	std::shared_ptr<Type> typeval;
	std::shared_ptr<Type> underlying_type; // note this is similar to TypeVarWalrus
};

struct TypeCallWithSym {
	std::shared_ptr<Type> target;
	std::string sym;
	std::shared_ptr<Type> underlying_type;
};

struct OpNumeric {
	std::string op;
	std::shared_ptr<Type> operand;
};

struct TypeExprMulti {
	std::vector<OpNumeric> ops;
	std::shared_ptr<Type> underlying_type;
};

struct TypeExprAddit {
	std::vector<OpNumeric> ops;
	std::shared_ptr<Type> underlying_type;
};

struct OpLogical {
	std::shared_ptr<Type> operand;
};

struct TypeExprLogicalAnd {
	std::vector<OpLogical> ops;
	std::shared_ptr<Type> underlying_type;
};

struct TypeExprLogicalOr {
	std::vector<OpLogical> ops;
	std::shared_ptr<Type> underlying_type;
};
