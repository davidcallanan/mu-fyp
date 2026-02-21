#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include "t_types_fwd.hpp"
#include "t_instructions_fwd.hpp"
#include "t_hardval_fwd.hpp"

struct TypeMap {
	std::shared_ptr<Type> leaf_type;
	std::shared_ptr<Hardval> leaf_hardval;
	std::shared_ptr<TypeMap> call_input_type;
	std::shared_ptr<TypeMap> call_output_type;
	std::map<std::string, std::shared_ptr<Type>> sym_inputs;
	std::vector<Instruction> execution_sequence;
};

struct TypePointer {
	std::shared_ptr<Type> target;
	std::shared_ptr<Hardval> hardval; // eventually this may need to be organized better, not sure how to best approach it.
};

struct TypeVarAccess {
	std::string target_name;
	std::shared_ptr<Type> underlying_type;
};

struct TypeMerged {
	std::vector<Type> types;
};

struct TypeRotten {
	std::string type_str;
};

struct TypeLog {
	std::shared_ptr<Type> message;
};

struct TypeAssign {
	std::string name;
	std::shared_ptr<Type> typeval;
	std::shared_ptr<Type> underlying_type; // note this is not the underlying type of the assignment, but rather the usage of assignment as an expression, which evaluates to a variable access - equivalent to underlying_type in TypeVarAccess.
};

struct TypeCallWithSym {
	std::shared_ptr<Type> target;
	std::string sym;
};
