#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>
#include "normalize_type.hpp"
#include "t_types.hpp"
#include "t_instructions.hpp"
#include "t_hardval.hpp"
#include "get_underlying_type.hpp"

using json = nlohmann::json;

Type normalize_type(
	const json& typeval,
	TypeSymbolTable& symbol_table
) {
	if (typeval.is_null()) {
		fprintf(stderr, "Got null for .type\n");
		exit(1);
	}
	
	if (!typeval.contains("type")) {
		fprintf(stderr, "Where is the .type field\n");
		exit(1);
	}
	
	std::string type = typeval["type"];
	
	if (type == "type_named") {
		if (!typeval.contains("trail")) {
			fprintf(stderr, "Expected .trail");
			exit(1);
		}
		
		std::string trail = typeval["trail"];
		TypeMap* found = symbol_table.get(trail);
		
		if (found == nullptr) {
			fprintf(stderr, "No type pre-given for this %s\n", trail.c_str());
			exit(1);
		}
		
		return std::make_shared<TypeMap>(*found);
	}
	
	if (type == "type_ptr") {
		if (!typeval.contains("target")) {
			fprintf(stderr, "Expected to see .target\n");
			exit(1);
		}
		
		Type target_type = normalize_type(typeval["target"], symbol_table);
		
		return std::make_shared<TypePointer>(TypePointer{
			std::make_shared<Type>(target_type),
			nullptr,
		});
	}
	
	if (type == "type_map") {
		TypeMap result;
		result.leaf_type = nullptr;
		result.leaf_hardval = nullptr;
		
		if (typeval.contains("leaf_type") && !typeval["leaf_type"].is_null()) {
			const auto& leaf_type_data = typeval["leaf_type"];
			
			if (!leaf_type_data.contains("type")) {
				fprintf(stderr, "Expected a .type\n");
				exit(1);
			}
			
			std::string leaf_type_type = leaf_type_data["type"];
			
			if (leaf_type_type == "type_named") {
				if (!leaf_type_data.contains("trail")) {
					fprintf(stderr, "Expected some .trail\n");
					exit(1);
				}
				
				auto v_rotten = std::make_shared<TypeRotten>();
				v_rotten->type_str = leaf_type_data["trail"];
				result.leaf_type = std::make_shared<Type>(v_rotten);
			} else {
				fprintf(stderr, "Type given was not handled, got %s\n", leaf_type_type.c_str());
				exit(1);
			}
		}
		
		if (typeval.contains("leaf_hardval") && !typeval["leaf_hardval"].is_null()) {
			const auto& hardval_data = typeval["leaf_hardval"];
			
			if (!hardval_data.contains("type")) {
				fprintf(stderr, "Expected to get some .type\n");
				exit(1);
			}
			
			std::string hardval_type = hardval_data["type"];
				
			if (hardval_type == "hardval_integer") {
				if (!hardval_data.contains("value")) {
					fprintf(stderr, "Expected .value\n");
					exit(1);
				}
				
				auto v_int = std::make_shared<HardvalInteger>();
				v_int->value = hardval_data["value"].get<std::string>();
				result.leaf_hardval = std::make_shared<Hardval>(v_int);
			} else if (hardval_type == "hardval_float") {
				if (!hardval_data.contains("value")) {
					fprintf(stderr, "Expected .value\n");
					exit(1);
				}
				
				auto v_float = std::make_shared<HardvalFloat>();
				v_float->value = hardval_data["value"].get<std::string>();
				result.leaf_hardval = std::make_shared<Hardval>(v_float);
			} else if (hardval_type == "hardval_string") {
				if (!hardval_data.contains("value")) {
					fprintf(stderr, "Expected .value\n");
					exit(1);
				}
				
				auto v_string = std::make_shared<HardvalString>();
				v_string->value = hardval_data["value"].get<std::string>();
				result.leaf_hardval = std::make_shared<Hardval>(v_string);
			} else {
				fprintf(stderr, "Unhandled situation %s\n", hardval_type.c_str());
				exit(1);
			}
		}

		result.call_input_type = (false
			|| !typeval.contains("call_input_type")
			|| typeval["call_input_type"].is_null()
		) ? nullptr : [&]() {
			Type t = normalize_type(typeval["call_input_type"], symbol_table);
			auto p_map = std::get_if<std::shared_ptr<TypeMap>>(&t);
			
			if (!p_map) {
				fprintf(stderr, "the input of a callable map must itself be a map\n");
				exit(1);
			}
			
			return std::make_unique<TypeMap>(**p_map);
		}();
		result.call_output_type = (false
			|| !typeval.contains("call_output_type")
			|| typeval["call_output_type"].is_null()
		) ? nullptr : [&]() {
			Type t = normalize_type(typeval["call_output_type"], symbol_table);
			auto p_map = std::get_if<std::shared_ptr<TypeMap>>(&t);
			if (!p_map) {
				fprintf(stderr, "the output of a callable map must itself be a map\n");
				exit(1);
			}
			return std::make_unique<TypeMap>(**p_map);
		}();
			
		if (typeval.contains("sym_inputs")) {
			if (!typeval["sym_inputs"].is_object()) {
				fprintf(stderr, "Expected .sym_inputs as object\n");
				exit(1);
			}
			
			// i think populating this later is ok.
			// for (auto& [key, value] : typeval["sym_inputs"].items()) { // don't really care about this for now
			// 	result.sym_inputs[key] = std::make_shared<Type>(normalize_type(value, symbol_table));
			// }
		}
		if (typeval.contains("instructions")) {
			if (!typeval["instructions"].is_array()) {
				fprintf(stderr, "Expected .instructions as array\n");
				exit(1);
			}
			for (const auto& instruction_data : typeval["instructions"]) {
				if (!instruction_data.contains("type")) {
					fprintf(stderr, "expected .type");
					exit(1);
				}
				
				std::string instruction_type = instruction_data["type"];
				
				if (instruction_type == "map_entry_expr") {
					auto v_expr = std::make_shared<InstructionExpr>();
					
					if (!instruction_data.contains("expr")) {
						fprintf(stderr, "Expected an actual .expr\n");
						exit(1);
					}
					
					Type actually_normalized = normalize_type(instruction_data["expr"], symbol_table);
					v_expr->expr = std::make_shared<Type>(actually_normalized);
					
					result.execution_sequence.push_back(v_expr);
					continue;
				}
				
				if (instruction_type == "map_entry_sym") {
					auto v_sym = std::make_shared<InstructionSym>();
					
					if (!instruction_data.contains("name")) {
						fprintf(stderr, "Expected .name\n");
						exit(1);
					}
					
					v_sym->name = instruction_data["name"].get<std::string>();
					
					if (!instruction_data.contains("typeval")) {
						fprintf(stderr, "Expected .typeval\n");
						exit(1);
					}
					
					Type normalized_type = normalize_type(instruction_data["typeval"], symbol_table);
					v_sym->typeval = std::make_shared<Type>(normalized_type);
					
					result.sym_inputs[v_sym->name] = v_sym->typeval;
					
					result.execution_sequence.push_back(v_sym);
					continue;
				}
			
				fprintf(stderr, "Not recognized instructoin type: %s\n", instruction_type.c_str());
				exit(1);
			}
		}
		
		return std::make_shared<TypeMap>(result);
	}
	
	if (type == "type_var_access") {
		if (!typeval.contains("target_name")) {
			fprintf(stderr, "Expected .target_name\n");
			exit(1);
		}
		
		std::string target_name = typeval["target_name"];
		auto v_var_access = std::make_shared<TypeVarAccess>();
		
		v_var_access->target_name = target_name;
		
		TypeMap* entry = symbol_table.get(target_name);
		
		if (entry == nullptr) {
			fprintf(stderr, "Var access failed to determine type information because consultation of symbol table failed on %s.\n", target_name.c_str());
			exit(1);
		}
		
		Type entry_as_type = std::make_shared<TypeMap>(*entry);
		const Type& underlying = get_underlying_type(entry_as_type);
		v_var_access->underlying_type = std::make_shared<Type>(underlying);
		
		return v_var_access;
	}
	
	if (type == "expr_assign") {
		auto v_assign = std::make_shared<TypeAssign>();
		
		if (!typeval.contains("name")) {
			fprintf(stderr, "expected .name\n");
			exit(1);
		}
		
		v_assign->name = typeval["name"].get<std::string>();
		
		if (!typeval.contains("typeval")) {
			fprintf(stderr, "expectred .typeval\n");
			exit(1);
		}
		
		Type delicious_type = normalize_type(typeval["typeval"], symbol_table);
		v_assign->typeval = std::make_shared<Type>(delicious_type);
		
		return v_assign;
	}
	
	if (type == "expr_log") {
		auto v_log = std::make_shared<TypeLog>();
		
		v_log->message = (false
			|| !typeval.contains("message")
			|| typeval["message"].is_null()
		) ? nullptr : std::make_shared<Type>(normalize_type(typeval["message"], symbol_table));
		
		return v_log;
	}
	
	if (type == "expr_call_with_sym") {
		auto v_call_with_sym = std::make_shared<TypeCallWithSym>();
		
		if (!typeval.contains("target")) {
			fprintf(stderr, "Where is this .target gone!\n");
			exit(1);
		}
		
		if (!typeval.contains("sym")) {
			fprintf(stderr, "Where is the .sym gone!\n");
			exit(1);
		}
		
		v_call_with_sym->target = std::make_shared<Type>(normalize_type(typeval["target"], symbol_table));
		v_call_with_sym->sym = typeval["sym"].get<std::string>();
		
		return v_call_with_sym;
	}
	
	if (type == "type_constrained") {
		if (false
			|| !typeval.contains("constraints")
			|| !typeval["constraints"].is_array()
		) {
			fprintf(stderr, "bad type_constrained, expected .constraints\n");
			exit(1);
		}
		
		const auto& constraints = typeval["constraints"];
		
		if (constraints.size() < 2) {
			fprintf(stderr, "useless constraints, only one or less given\n");
			exit(1);
		}
		
		if (constraints.size() > 2) {
			fprintf(stderr, "useful constraints, but more than 2 not handled yet\n");
			exit(1);
		}
		
		Type constraint_1 = normalize_type(constraints[0], symbol_table);
		Type constraint_2 = normalize_type(constraints[1], symbol_table);
		
		if (auto p_v_1 = std::get_if<std::shared_ptr<TypePointer>>(&constraint_1)) {
			auto p_v_2 = std::get_if<std::shared_ptr<TypeMap>>(&constraint_2);
			
			if (!p_v_2) {
				fprintf(stderr, "In case that first constraint is pointer, for now only second constraint as map is supported.\n");
				exit(1);
			}
			
			TypeMap& v_2 = **p_v_2;
			
			if (v_2.leaf_hardval == nullptr) {
				fprintf(stderr, "For now constrained pointers must have definitive value associated with them\n");
				exit(1);
			}
			
			auto p_pointer = std::make_shared<TypePointer>();
			
			p_pointer->target = (*p_v_1)->target;
			p_pointer->hardval = v_2.leaf_hardval;
			
			return p_pointer;
		}
		
		auto p_v_1 = std::get_if<std::shared_ptr<TypeMap>>(&constraint_1);
		auto p_v_2 = std::get_if<std::shared_ptr<TypeMap>>(&constraint_2);
		
		if (!p_v_1 || !p_v_2) {
			// fprintf(stderr, "In case that first constraint is not a pointer, expected both constrained types to be maps, for now.\n");
			// exit(1);
		}
		
		if (p_v_1 && p_v_2) {
			auto v_merged = std::make_shared<TypeMerged>();
			
			v_merged->types.push_back(constraint_1);
			v_merged->types.push_back(constraint_2);
			
			return v_merged;
		}
		
		auto v_merged = std::make_shared<TypeMerged>();
		
		v_merged->types.push_back(constraint_1);
		v_merged->types.push_back(constraint_2);
		
		return v_merged;
	}
	
	fprintf(stderr, "unhandled type, got %s\n", type.c_str());
	exit(1);
}
