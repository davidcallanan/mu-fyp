#include <cstdio>
#include <fstream>
#include <string>
#include "dependencies/json.hpp"
#include "create_type_symbol_table.hpp"
#include "create_value_symbol_table.hpp"
#include "evaluate_hardval.hpp"
#include "evaluate_structval.hpp"
#include "process_map_body.hpp"
#include "t_hardval.hpp"
#include "t_instructions.hpp"
#include "t_ctx.hpp"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/TargetParser/Host.h"

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
				
				if (instruction_type == "map_entry_log") {
					auto v_log = std::make_shared<InstructionLog>();
					
					v_log->message = (false
						|| !instruction_data.contains("message")
						|| instruction_data["message"].is_null()
					) ? nullptr : std::make_shared<Type>(normalize_type(instruction_data["message"], symbol_table));
					
					result.execution_sequence.push_back(v_log);
					
					continue;
				}
				
				if (instruction_type == "map_entry_assign") {
					auto v_assign = std::make_shared<InstructionAssign>();
					
					if (!instruction_data.contains("name")) {
						fprintf(stderr, "Expected .name\n");
						exit(1);
					}
					
					
					v_assign->name = instruction_data["name"].get<std::string>();
					
					if (!instruction_data.contains("typeval")) {
						fprintf(stderr, "Expected .typeval\n");
						exit(1);
					}
					
					Type normalized_type = normalize_type(instruction_data["typeval"], symbol_table);
					v_assign->typeval = std::make_shared<Type>(normalized_type);
					
					result.execution_sequence.push_back(v_assign);
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
		
		return std::make_shared<TypeVarAccess>(TypeVarAccess{
			target_name,
		});
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

void gen_module_binary(const json& create_data, TypeSymbolTable& symbol_table) {
	// the amount of boilerplate is crazy lol
	printf("Generating module binary!!\n");
	
	llvm::LLVMContext context;
	llvm::Module module("foobar", context);
	llvm::IRBuilder<> builder(context);
	
	auto target_triple = llvm::sys::getDefaultTargetTriple();
	module.setTargetTriple(target_triple);
	
	std::vector<llvm::Type*> puts_args = { llvm::Type::getInt8PtrTy(context) };
	
	llvm::FunctionType* puts_type = llvm::FunctionType::get(
		llvm::Type::getInt32Ty(context),
		puts_args,
		false
	);
	
	llvm::FunctionCallee puts_func = module.getOrInsertFunction("puts", puts_type);
	
	llvm::FunctionType* main_type = llvm::FunctionType::get(
		llvm::Type::getInt32Ty(context),
		false
	);
	
	llvm::Function* main_func = llvm::Function::Create(
		main_type,
		llvm::Function::ExternalLinkage,
		"main",
		module
	);
	
	llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", main_func);
	builder.SetInsertPoint(entry);
	
	const char* message;
	
	if (create_data.is_null()) {
		message = "No module entrypoint";
	} else {
		message = "Module entrypoint found";
	}
	
	llvm::Value* message_str = builder.CreateGlobalStringPtr(message);
	builder.CreateCall(puts_func, { message_str });
	
	if (!create_data.is_null()) {
		if (!create_data.contains("description")) {
			fprintf(stderr, "Where is .description gone\n");
			exit(1);
		}
		
		const json& description = create_data["description"];

		Type normalized = normalize_type(description, symbol_table);
		auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&normalized);
		if (!p_v_map) {
			fprintf(stderr, "description of create must be map, not something else.\n");
			exit(1);
		}
		TypeMap v_map = **p_v_map;

		if (v_map.call_output_type == nullptr) {
			fprintf(stderr, "at the moment we need an output which I suppose makes sense for a create block\n");
			exit(1);
		}
		
		ValueSymbolTable value_table = create_value_symbol_table();
		
		IrGenCtx igc = {
			context,
			module,
			builder,
			symbol_table,
			value_table,
			puts_func,
		};
		
		process_map_body(igc, *v_map.call_output_type);
	}
	
	builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));
	
	std::string verify_error;
	llvm::raw_string_ostream verify_stream(verify_error);
	
	if (llvm::verifyModule(module, &verify_stream)) {
		fprintf(stderr, "Verification of generated module did not go smoothly, because:%s\n", verify_error.c_str());
		exit(1);
	}
	
	printf("Module generation went fantastically.\n");
	
	std::error_code error_code;
	llvm::raw_fd_ostream ir_file("/app/out/hello.ll", error_code, llvm::sys::fs::OF_None);
	
	if (error_code) {
		fprintf(stderr, "Opening file error: %s\n", error_code.message().c_str());
		exit(1);
	}
	
	module.print(ir_file, nullptr);
	ir_file.close();
	
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();
	
	std::string error;
	const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);
	
	if (!target) {
		fprintf(stderr, "Couldn't locate desired target because %s\n", error.c_str());
		exit(1);
	}
	
	llvm::TargetOptions opt;
	
	llvm::TargetMachine* target_machine = target->createTargetMachine(
		target_triple,
		"generic",
		"",
		opt,
		llvm::Reloc::PIC_
	);
	
	module.setDataLayout(target_machine->createDataLayout());
	
	llvm::raw_fd_ostream obj_file("/app/out/hello.o", error_code, llvm::sys::fs::OF_None);
	
	if (error_code) {
		fprintf(stderr, "Opening file error because %s\n", error_code.message().c_str());
		delete target_machine;
		exit(1);
	}
	
	llvm::legacy::PassManager pass_manager;
	
	if (target_machine->addPassesToEmitFile(pass_manager, obj_file, nullptr, llvm::CodeGenFileType::CGFT_ObjectFile)) {
		fprintf(stderr, "Object file emission not supported for some reason.\n");
		delete target_machine;
		exit(1);
	}
	
	pass_manager.run(module);
	obj_file.close();
	
	printf("All done!");
	
	delete target_machine;
}

int main(int argc, char* argv[]) {
	std::string json_path = "/volume/in/frontend.out.json";

	printf("Obtaining json frontend result from %s\n", json_path.c_str());

	std::ifstream json_file(json_path);
	
	if (!json_file) {
		fprintf(stderr, "Error opening %s\n", json_path.c_str());
		return 1;
	}

	json frontend_data;
	
	try {
		json_file >> frontend_data;
	} catch (const json::parse_error& e) {
		fprintf(stderr, "JSON could not parse, because %s\n", e.what());
		return 1;
	}
	
	json_file.close();

	printf("json has been smoothly parsed.\n");

	auto& parse_output = frontend_data["parse_output"];
	
	TypeSymbolTable symbol_table = create_type_symbol_table();
	
	json create_block = nullptr;
	if (parse_output.contains("create") && !parse_output["create"].is_null()) {
		create_block = parse_output["create"];
	}
	
	gen_module_binary(create_block, symbol_table);

	auto& dir_node_translations = frontend_data["dir_node_translations"];
	
	for (auto& [uuid, node_data] : dir_node_translations.items()) {
		printf("%s -> %s\n", uuid.c_str(), node_data["path"].get<std::string>().c_str());
	}

	return 0;
}
