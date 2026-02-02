#include <cstdio>
#include <fstream>
#include <string>
#include "dependencies/json.hpp"
#include "create_type_symbol_table.hpp"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/TargetParser/Host.h"

using json = nlohmann::json;

TypeMap normalize_to_map(
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
		
		return *found;
	}
	
	if (type == "type_map") {
		TypeMap result;
		result.leaf_type = "";
		result.call_input_type = (false
			|| !typeval.contains("call_input_type")
			|| typeval["call_input_type"].is_null()
		) ? nullptr : std::make_unique<TypeMap>(normalize_to_map(typeval["call_input_type"], symbol_table));
		result.call_output_type = (false
			|| !typeval.contains("call_output_type")
			|| typeval["call_output_type"].is_null()
		) ? nullptr : std::make_unique<TypeMap>(normalize_to_map(typeval["call_output_type"], symbol_table));
			
		if (typeval.contains("sym_inputs")) {
			if (!typeval["sym_inputs"].is_object()) {
				fprintf(stderr, "Expected .sym_inputs as object\n");
				exit(1);
			}
			
			for (auto& [key, value] : typeval["sym_inputs"].items()) { // don't really care about this for now
				result.sym_inputs[key] = std::make_shared<Type>(normalize_to_map(value, symbol_table));
			}
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
					InstructionLog v_log;
					
					v_log.message = (false
						|| !instruction_data.contains("message")
						|| instruction_data["message"].is_null()
					) ? "" : instruction_data["message"].get<std::string>();
					
					result.execution_sequence.push_back(v_log);
					
					continue;
				}
				
				fprintf(stderr, "Not recognized instructoin type: %s\n", instruction_type.c_str());
				exit(1);
			}
		}
		
		return result;
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
		
		// for now just return the second constraint, need to eventually implement merge process
		
		if (constraints.size() >= 2) {
			return normalize_to_map(constraints[1], symbol_table);
		} else {
			fprintf(stderr, "useless constraints, only one or less given\n");
			exit(1);
		}
	}
	
	fprintf(stderr, "unhandled type, got %s\n", type.c_str());
	exit(1);
}

void process_map_body(
	llvm::IRBuilder<>& builder,
	llvm::FunctionCallee& puts_func,
	const TypeMap& body
) {
	for (const auto& instruction : body.execution_sequence) {
		if (std::holds_alternative<InstructionLog>(instruction)) {
			const InstructionLog& v_log = std::get<InstructionLog>(instruction);
			llvm::Value* log_str = builder.CreateGlobalStringPtr(v_log.message);
			builder.CreateCall(puts_func, { log_str });
		}
	}
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
		
		TypeMap normalized = normalize_to_map(description, symbol_table);
		
		if (normalized.call_output_type == nullptr) {
			fprintf(stderr, "at the moment we need an output which I suppose makes sense for a create block\n");
			exit(1);
		}
		
		process_map_body(builder, puts_func, *normalized.call_output_type);
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
