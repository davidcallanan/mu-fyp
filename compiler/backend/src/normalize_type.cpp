#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <memory>
#include "normalize_type.hpp"
#include "demote_underlying.hpp"
#include "merge_underlying_type.hpp"
#include "get_underlying_type.hpp"
#include "t_types.hpp"
#include "t_instructions.hpp"
#include "t_hardval.hpp"
#include "t_bundles.hpp"
#include "preinstantiated_types.hpp"
#include "rotten_int_info.hpp"
#include "rotten_float_info.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringRef.h"

using json = nlohmann::json;

Type normalize_type(
	TypeOrchCtx& toc,
	const json& typeval
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
		std::optional<UnderlyingType> found = toc.type_table->get(trail);
		
		if (!found.has_value()) {
			fprintf(stderr, "No type pre-given for this %s\n", trail.c_str());
			exit(1);
		}
		
		return demote_underlying(found.value());
	}
	
	if (type == "type_ptr") {
		if (!typeval.contains("target")) {
			fprintf(stderr, "Expected to see .target\n");
			exit(1);
		}
		
		Type target_type = normalize_type(toc, typeval["target"]);
		
		return std::make_shared<TypePointer>(TypePointer{
			std::make_shared<Type>(target_type),
			std::nullopt,
		});
	}
	
	if (type == "type_map") {
		TypeMap result;
		result.leaf_type = std::nullopt;
		result.leaf_hardval = std::nullopt;
		
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
				result.leaf_type = Type(v_rotten);
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

				if (!result.leaf_type.has_value()) { // this is duplicating logic, but it's too difficult to get the code working without forcing the leaf_type early on.
					const std::string& value_str = hardval_data["value"].get_ref<const std::string&>();
					bool is_negative = !value_str.empty() && value_str[0] == '-';
					std::string digits = is_negative ? value_str.substr(1) : value_str;
					
					uint32_t scratchy_bits = (uint32_t)(digits.size() * 4) + 1;
					llvm::APInt ap_value(scratchy_bits, digits.c_str(), 10);

					int bits_needed = 0;

					if (is_negative) {
						bits_needed = ap_value.getActiveBits() + 1; // i'm not entirely sure if this is correct.
					} else {
						bits_needed = ap_value.getActiveBits();

						if (bits_needed == 0) {
							bits_needed = 1;
						}
					}

					auto v_rotten = std::make_shared<TypeRotten>();
					v_rotten->type_str = (is_negative ? "i" : "u") + std::to_string(bits_needed);
					result.leaf_type = Type(v_rotten);
				}

				auto v_int = std::make_shared<HardvalInteger>();
				v_int->value = hardval_data["value"].get<std::string>();
				result.leaf_hardval = Hardval(v_int);
			} else if (hardval_type == "hardval_float") {
				if (!hardval_data.contains("value")) {
					fprintf(stderr, "Expected .value\n");
					exit(1);
				}

				if (!result.leaf_type.has_value()) {
					auto v_rotten = std::make_shared<TypeRotten>();
					v_rotten->type_str = "f64"; // I don't have time to sort this, assuming all rottens are 64-bit floats which is fine for now.
					result.leaf_type = Type(v_rotten);
				}

				auto v_float = std::make_shared<HardvalFloat>();
				v_float->value = hardval_data["value"].get<std::string>();
				result.leaf_hardval = Hardval(v_float);
			} else if (hardval_type == "hardval_string") {
				if (!hardval_data.contains("value")) {
					fprintf(stderr, "Expected .value\n");
					exit(1);
				}

				if (!result.leaf_type.has_value()) {
					auto v_rotten = std::make_shared<TypeRotten>();
					v_rotten->type_str = "u8";
					
					auto v_pointer = std::make_shared<TypePointer>();
					v_pointer->target = std::make_shared<Type>(Type(v_rotten));
					v_pointer->hardval = std::nullopt;
					
					result.leaf_type = Type(v_pointer);
				}

				auto v_string = std::make_shared<HardvalString>();
				v_string->value = hardval_data["value"].get<std::string>();
				result.leaf_hardval = Hardval(v_string);
			} else {
				fprintf(stderr, "Unhandled situation %s\n", hardval_type.c_str());
				exit(1);
			}
		}

		result.call_input_identifier = (false
			|| !typeval.contains("call_input_identifier")
			|| typeval["call_input_identifier"].is_null()
		) ? std::nullopt : std::optional<std::string>(typeval["call_input_identifier"].get<std::string>());
		
		result.call_input_type = (false
			|| !typeval.contains("call_input_type")
			|| typeval["call_input_type"].is_null()
		) ? nullptr : [&]() {
			Type t = normalize_type(toc, typeval["call_input_type"]);
			auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&t);
			
			if (!p_v_map) {
				fprintf(stderr, "the input of a callable map must itself be a map\n");
				exit(1);
			}
			
			return std::make_unique<TypeMap>(**p_v_map);
		}();
		
		result.call_output_type = (false
			|| !typeval.contains("call_output_type")
			|| typeval["call_output_type"].is_null()
		) ? nullptr : [&]() {
			Type t = normalize_type(toc, typeval["call_output_type"]);
			auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&t);
			
			if (!p_v_map) {
				fprintf(stderr, "the output of a callable map must itself be a map\n");
				exit(1);
			}
			
			return std::make_unique<TypeMap>(**p_v_map);
		}();

		// bundles are now always assigned - we need them to keep track of the opaque struct type!
		if (true
			// && result.call_input_type != nullptr
			// && result.call_output_type != nullptr
		) {
			result.bundle_id = toc.bundle_registry->install(
				std::make_shared<BundleMap>(BundleMap{ nullptr, nullptr, nullptr }
			));
		}
		
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
					
					Type actually_normalized = normalize_type(toc, instruction_data["expr"]);
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
					
					Type normalized_type = normalize_type(toc, instruction_data["typeval"]);
					v_sym->typeval = std::make_shared<Type>(normalized_type);
					
					result.sym_inputs[v_sym->name] = v_sym->typeval;
					
					result.execution_sequence.push_back(v_sym);
					continue;
				}
			
				if (instruction_type == "instruction_for") {
					auto v_for = std::make_shared<InstructionFor>();

					if (!instruction_data.contains("body")) {
						fprintf(stderr, "Expecting .body\n");
						exit(1);
					}

					Type body = normalize_type(toc, instruction_data["body"]);
					auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&body);

					if (!p_v_map) {
						fprintf(stderr, "Somehow parser allowed non-map body for my loop.\n");
						exit(1);
					}

					v_for->body = *p_v_map;
					result.execution_sequence.push_back(v_for);
					
					continue;
				}

				if (instruction_type == "instruction_if") {
					auto v_if = std::make_shared<InstructionIf>();

					if (!instruction_data.contains("branches") || !instruction_data["branches"].is_array()) {
						fprintf(stderr, ".branches missing.\n");
						exit(1);
					}

					for (const auto& branch : instruction_data["branches"]) {
						InstructionIf_Branch actual_branch;

						if (!branch.contains("body")) {
							fprintf(stderr, "Branch lacks a .body.\n");
							exit(1);
						}

						if (branch.contains("condition") && !branch["condition"].is_null()) {
							Type condition = normalize_type(toc, branch["condition"]);
							actual_branch.condition = std::make_shared<Type>(condition);
						}

						Type body = normalize_type(toc, branch["body"]);
						auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&body);

						if (!p_v_map) {
							fprintf(stderr, "frontend did not produce a map here.\n");
							exit(1);
						}

						actual_branch.body = *p_v_map;
						v_if->branches.push_back(actual_branch);
					}

					result.execution_sequence.push_back(v_if);
					
					continue;
				}

				if (instruction_type == "instruction_break") {
					auto v_break = std::make_shared<InstructionBreak>();
					
					result.execution_sequence.push_back(v_break);
					
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
		
		return v_var_access;
	}
	
	if (type == "expr_var_walrus") {
		auto v_var_walrus = std::make_shared<TypeVarWalrus>();
		
		if (!typeval.contains("name")) {
			fprintf(stderr, "expected .name\n");
			exit(1);
		}
		
		v_var_walrus->name = typeval["name"].get<std::string>();
		v_var_walrus->is_mut = typeval.value("is_mut", false);
		
		if (!typeval.contains("typeval")) {
			fprintf(stderr, "expected .typeval\n");
			exit(1);
		}
		
		Type delicious_type = normalize_type(toc, typeval["typeval"]);
		v_var_walrus->typeval = std::make_shared<Type>(delicious_type);
		
		return v_var_walrus;
	}
	
	if (type == "expr_var_assign") {
		auto v_var_assign = std::make_shared<TypeVarAssign>();
		
		if (!typeval.contains("name")) {
			fprintf(stderr, "expected .name\n");
			exit(1);
		}
		
		v_var_assign->name = typeval["name"].get<std::string>();
		
		if (!typeval.contains("typeval")) {
			fprintf(stderr, "expectred .typevla\n");
			exit(1);
		}
		
		Type delicious_type = normalize_type(toc, typeval["typeval"]);
		v_var_assign->typeval = std::make_shared<Type>(delicious_type);
		
		return v_var_assign;
	}
	
	if (type == "expr_log") {
		auto v_log = std::make_shared<TypeLog>();
		
		v_log->message = (false
			|| !typeval.contains("message")
			|| typeval["message"].is_null()
		) ? nullptr : std::make_shared<Type>(normalize_type(toc, typeval["message"]));
		
		return v_log;
	}
	
	if (type == "expr_log_d") {
		auto v_log_d = std::make_shared<TypeLogD>();

		if (!typeval.contains("message") || typeval["message"].is_null()) {
			fprintf(stderr, "you have to actually log something when using log_d, not provide no arguments\n");
			exit(1);
		}

		v_log_d->message = std::make_shared<Type>(normalize_type(toc, typeval["message"]));
		
		return v_log_d;
	}

	if (type == "expr_log_dd") {
		auto v_log_dd = std::make_shared<TypeLogDd>();

		if (!typeval.contains("message") || typeval["message"].is_null()) {
			fprintf(stderr, "have to provide a message to log_dd\n");
			exit(1);
		}

		if (!typeval.contains("byte_count") || typeval["byte_count"].is_null()) {
			fprintf(stderr, "have to provide a byte_count to log_dd or specify null-term\n");
			exit(1);
		}

		v_log_dd->message = std::make_shared<Type>(normalize_type(toc, typeval["message"]));

		const auto& byte_count = typeval["byte_count"];
		const std::string byte_count_type = byte_count["type"].get<std::string>();

		if (byte_count_type == "nullterm") {
			v_log_dd->is_nullterm = true;
			v_log_dd->byte_count = nullptr;
		} else if (byte_count_type == "byte_count") {
			v_log_dd->is_nullterm = false;
			v_log_dd->byte_count = std::make_shared<Type>(normalize_type(toc, byte_count["count"]));
		} else {
			fprintf(stderr, "bizarre type for byte_count %s\n", byte_count_type.c_str());
			exit(1);
		}

		return v_log_dd;
	}

	if (type == "type_enum") {
		if (!typeval.contains("hardsym") && !typeval.contains("syms")) {
			fprintf(stderr, "A definitive enum instantiation requires an actual symbol or a list of acceptable possibilities.\n");
			exit(1);
		}

		auto v_enum = std::make_shared<TypeEnum>();

		if (typeval.contains("syms") && typeval["syms"].is_array()) {
			for (const auto& sym_json : typeval["syms"]) {
				v_enum->syms.push_back(sym_json.get<std::string>());
			}
		}

		if (typeval.contains("hardsym")) {
			std::string hardsym = typeval["hardsym"].get<std::string>();
			v_enum->hardsym = hardsym;
			v_enum->is_instantiated = true;
			v_enum->syms.push_back(hardsym);
		}

		return v_enum;
	}

	if (type == "expr_call_with_sym") {
		if (!typeval.contains("target")) {
			fprintf(stderr, "Where is this .target gone!\n");
			exit(1);
		}
		
		if (!typeval.contains("sym")) {
			fprintf(stderr, "Where is the .sym gone!\n");
			exit(1);
		}

		Type target = normalize_type(toc, typeval["target"]);
		std::string sym = typeval["sym"].get<std::string>();

		if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&target)) {
			if (!(*p_v_enum)->hardsym.has_value()) {
				// the syntax is not actually a call in this case, but rather an instantiation.

				bool was_found = std::find((*p_v_enum)->syms.begin(), (*p_v_enum)->syms.end(), sym) != (*p_v_enum)->syms.end();

				if (!was_found) {
					fprintf(stderr, "Cannot instantiate %s as it is not one of the predefined possibilitirets.\n", sym.c_str());
					exit(1);
				}

				auto v_enum_with_hardsym = std::make_shared<TypeEnum>();
				v_enum_with_hardsym->hardsym = sym;
				v_enum_with_hardsym->is_instantiated = true;
				v_enum_with_hardsym->syms.push_back(sym);

				auto v_merged = std::make_shared<TypeMerged>();
				v_merged->types.push_back(target);
				v_merged->types.push_back(v_enum_with_hardsym);
				v_merged->underlying_type = std::make_shared<Type>(merge_underlying_type(target, Type(v_enum_with_hardsym)));

				return v_merged;
			}
		}

		auto v_call_with_sym = std::make_shared<TypeCallWithSym>();
		v_call_with_sym->target = std::make_shared<Type>(target);
		v_call_with_sym->sym = sym;
		
		return v_call_with_sym;
	}

	if (type == "expr_call_with_dynamic") {
		if (!typeval.contains("target")) {
			fprintf(stderr, "No .target was provoided.\n");
			exit(1);
		}

		if (!typeval.contains("call_data")) {
			fprintf(stderr, "No .call_data was provided.\n");
			exit(1);
		}

		Type target = normalize_type(toc, typeval["target"]);
		Type call_data = normalize_type(toc, typeval["call_data"]);

		auto v_call_with_dynamic = std::make_shared<TypeCallWithDynamic>();
		v_call_with_dynamic->target = std::make_shared<Type>(target);
		v_call_with_dynamic->call_data = std::make_shared<Type>(call_data);
		v_call_with_dynamic->is_flag_alwaysinline = typeval.value("is_flag_alwaysinline", false);

		return v_call_with_dynamic;
	}
	
	if (type == "expr_multi") {
		auto v_expr_multi = std::make_shared<TypeExprMulti>();
		
		for (const auto& op_data : typeval["ops"]) {
			OpNumeric op;
			op.op = op_data["op"].get<std::string>();
			op.operand = std::make_shared<Type>(normalize_type(toc, op_data["operand"]));
			v_expr_multi->ops.push_back(op);
		}

		if (v_expr_multi->ops.empty()) {
			fprintf(stderr, "this is quite bizrree.\n");
			exit(1);
		}

		return v_expr_multi;
	}

	if (type == "expr_addit") {
		auto v_expr_addit = std::make_shared<TypeExprAddit>();
		
		for (const auto& op_data : typeval["ops"]) {
			OpNumeric op;
			op.op = op_data["op"].get<std::string>();
			op.operand = std::make_shared<Type>(normalize_type(toc, op_data["operand"]));
			v_expr_addit->ops.push_back(op);
		}

		if (v_expr_addit->ops.empty()) {
			fprintf(stderr, "this is not good outcome.\n");
			exit(1);
		}

		return v_expr_addit;
	}
	
	if (type == "expr_logical_and") {
		auto v_expr_logical_and = std::make_shared<TypeExprLogicalAnd>();

		for (const auto& op_data : typeval["ops"]) {
			OpLogical op;
			op.operand = std::make_shared<Type>(normalize_type(toc, op_data["operand"]));
			v_expr_logical_and->ops.push_back(op);
		}

		v_expr_logical_and->underlying_type = std::make_shared<Type>(Type(type_bool));

		return v_expr_logical_and;
	}

	if (type == "expr_logical_or") {
		auto v_expr_logical_or = std::make_shared<TypeExprLogicalOr>();

		for (const auto& op_data : typeval["ops"]) {
			OpLogical op;
			op.operand = std::make_shared<Type>(normalize_type(toc, op_data["operand"]));
			v_expr_logical_or->ops.push_back(op);
		}

		v_expr_logical_or->underlying_type = std::make_shared<Type>(Type(type_bool));

		return v_expr_logical_or;
	}
	
	if (type == "expr_compare") {
		auto v_compare = std::make_shared<TypeCompare>();

		v_compare->operator_ = typeval["operator"].get<std::string>();
		v_compare->operand_a = std::make_shared<Type>(normalize_type(toc, typeval["operand_a"]));
		v_compare->operand_b = std::make_shared<Type>(normalize_type(toc, typeval["operand_b"]));
		v_compare->underlying_type = std::make_shared<Type>(Type(type_bool));

		return v_compare;
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
		
		Type constraint_1 = normalize_type(toc, constraints[0]);
		Type constraint_2 = normalize_type(toc, constraints[1]);
		
		if (auto p_v_1 = std::get_if<std::shared_ptr<TypePointer>>(&constraint_1)) {
			auto p_v_2 = std::get_if<std::shared_ptr<TypeMap>>(&constraint_2);
			
			if (!p_v_2) {
				fprintf(stderr, "In case that first constraint is pointer, for now only second constraint as map is supported.\n");
				exit(1);
			}
			
			TypeMap& v_2 = **p_v_2;
			
			if (!v_2.leaf_hardval.has_value()) {
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
			v_merged->underlying_type = std::make_shared<Type>(merge_underlying_type(constraint_1, constraint_2));
			
			return v_merged;
		}
		
		auto v_merged = std::make_shared<TypeMerged>();
		
		v_merged->types.push_back(constraint_1);
		v_merged->types.push_back(constraint_2);
		v_merged->underlying_type = std::make_shared<Type>(merge_underlying_type(constraint_1, constraint_2));
		
		return v_merged;
	}
	
	if (type == "type_extern_ccc") {
		if (!typeval.contains("function_name")) {
			fprintf(stderr, ".function_name missing...\n");
			exit(1);
		}

		if (!typeval.contains("call_input_type") || typeval["call_input_type"].is_null()) {
			fprintf(stderr, ".call_input_type missing...\n");
			exit(1);
		}

		auto v_extern_ccc = std::make_shared<TypeExternCcc>();
		
		v_extern_ccc->function_name = typeval["function_name"].get<std::string>();

		Type normalized = normalize_type(toc, typeval["call_input_type"]);
		
		auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&normalized);

		if (!p_v_map) {
			fprintf(stderr, "input to an external map must itself be a map.\n");
			exit(1);
		}

		v_extern_ccc->call_input_type = *p_v_map;

		if (!typeval.contains("call_output_type") || typeval["call_output_type"].is_null()) {
			fprintf(stderr, ".call_output_type wasn't present.\n");
			exit(1);
		}

		Type output_normalized = normalize_type(toc, typeval["call_output_type"]);
		
		auto p_v_output_map = std::get_if<std::shared_ptr<TypeMap>>(&output_normalized);

		if (!p_v_output_map) {
			fprintf(stderr, "output must be map contatining one :0 field.\n");
			exit(1);
		}

		const auto& sym_inputs = (*p_v_output_map)->sym_inputs;

		if (sym_inputs.size() > 1) {
			fprintf(stderr, "at most one field is permitted to be returned due to C ABI reasons. If the actual function has multiple return values, you must troubleshoot how this is translated into the C ABI. You likely need to pass pointers!\n");
			exit(1);
		}

		if (sym_inputs.size() == 1) {
			const auto& [field_name, field_type] = *sym_inputs.begin();

			if (field_name != ":0") {
				fprintf(stderr, "the only field must be the :0 sym. found instead ..%s..\n", field_name.c_str());
				exit(1);
			}

			Type field_underlying = get_underlying_type(*field_type);

			if (auto p_v_field_map = std::get_if<std::shared_ptr<TypeMap>>(&field_underlying)) {
				if ((*p_v_field_map)->leaf_type.has_value()) {
					field_underlying = get_underlying_type((*p_v_field_map)->leaf_type.value());
				}
			}

			auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&field_underlying);
			auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&field_underlying);

			if (!p_v_rotten && !p_v_pointer) {
				fprintf(stderr, "Only permitted return types due to C ABI reasons are numerical data types (int, float) up to 64 bit, or pointers. For complex structures, you must follow the C ABI approach of passing pointers!\n");
				exit(1);
			}

			if (p_v_rotten) {
				auto int_info = rotten_int_info(*p_v_rotten);
				auto float_info = rotten_float_info(*p_v_rotten);
				
				const bool up_to_scratch = (false
					|| (int_info.has_value() && int_info->bits <= 64)
					|| (float_info.has_value() && float_info->bits <= 64)
				);
				
				if (!up_to_scratch) {
					fprintf(stderr, "the type ..%s.. is not permitted because it has a risk of not being C-abi compatible. use int or float less than 64 bit, or pointers.\n", (*p_v_rotten)->type_str.c_str());
					exit(1);
				}
			}
		}

		v_extern_ccc->call_output_type = *p_v_output_map;

		return v_extern_ccc;
	}

	if (type == "type_take_address") {
		if (!typeval.contains("target")) {
			fprintf(stderr, "Missings .target field!\n");
			exit(1);
		}

		auto v_take_address = std::make_shared<TypeTakeAddress>();
		
		v_take_address->target = std::make_shared<Type>(normalize_type(toc, typeval["target"]));

		return v_take_address;
	}

	if (type == "expr_sizeof") {
		if (!typeval.contains("target")) {
			fprintf(stderr, ".target went missing\n");
			exit(1);
		}

		auto v_sizeof = std::make_shared<TypeSizeof>();
		v_sizeof->target = std::make_shared<Type>(normalize_type(toc, typeval["target"]));

		auto v_rotten = std::make_shared<TypeRotten>();
		v_rotten->type_str = "u64";
		v_sizeof->underlying_type = std::make_shared<Type>(Type(v_rotten));

		return v_sizeof;
	}

	fprintf(stderr, "unhandled type, got %s\n", type.c_str());
	exit(1);
}
