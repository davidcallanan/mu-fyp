#include <cstdio>
#include <fstream>
#include <string>
#include "dependencies/json.hpp"

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

void process_map_entry(
	llvm::IRBuilder<>& builder,
	llvm::FunctionCallee& puts_func,
	const json& entry
) {
	if (entry.contains("type") && entry["type"] == "map_entry_log") {
		std::string log_message;
		
		if (entry.contains("message") && !entry["message"].is_null()) {
			log_message = entry["message"].get<std::string>();
		} else {
			log_message = "";
		}
		
		llvm::Value* log_str = builder.CreateGlobalStringPtr(log_message);
		builder.CreateCall(puts_func, { log_str });
	}
}

void process_map_body(
	llvm::IRBuilder<>& builder,
	llvm::FunctionCallee& puts_func,
	const json& body
) {
	if (!body.contains("type") || body["type"] != "constraint_map") {
		fprintf(stderr, "Expected .type == \"constraint_map\"\n");
		exit(1);
	}
	
	if (!body.contains("entries")) {
		fprintf(stderr, "Expected .entries\n");
		exit(1);
	}
	
	auto& entries = body["entries"];
	
	for (auto& entry : entries) {
		process_map_entry(builder, puts_func, entry);
	}
}

void gen_module_binary(const json& create_data) {
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
	
	if (true
		&& !create_data.is_null()
		&& create_data.contains("description")
		&& create_data["description"].contains("call_output_constraint")
	) {
		process_map_body(builder, puts_func, create_data["description"]["call_output_constraint"]);
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
	
	json create_block = nullptr;
	if (parse_output.contains("create") && !parse_output["create"].is_null()) {
		create_block = parse_output["create"];
	}
	
	gen_module_binary(create_block);

	auto& dir_node_translations = frontend_data["dir_node_translations"];
	
	for (auto& [uuid, node_data] : dir_node_translations.items()) {
		printf("%s -> %s\n", uuid.c_str(), node_data["path"].get<std::string>().c_str());
	}

	return 0;
}
