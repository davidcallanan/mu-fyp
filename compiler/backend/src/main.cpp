#include <cstdio>
#include <fstream>
#include <string>
#include "dependencies/json.hpp"
#include "create_type_symbol_table.hpp"
#include "create_value_symbol_table.hpp"
#include "evaluate_hardval.hpp"
#include "evaluate_structval.hpp"
#include "process_map_body.hpp"
#include "normalize_type.hpp"
#include "promote_to_underlying.hpp"
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
	
	// Takes in (i64 chunk, i8 bl, i8 br).
	llvm::Function* log_data_func = [&]() -> llvm::Function* {
		// there's only one worse thing than handwriting an LLVM IR function, and that's handwiriting C++ code that generates one lol.
		// but by relying on handrwritten functions just for debugging functions (like log_d), we don't depend on the language's interoperability layer to be in working condition, allowing interoparability to be developed at a later stage.
		
		llvm::Type* i8  = llvm::Type::getInt8Ty(context);
		llvm::Type* i32 = llvm::Type::getInt32Ty(context);
		llvm::Type* i64 = llvm::Type::getInt64Ty(context);

		llvm::FunctionType* fn_type = llvm::FunctionType::get(
			llvm::Type::getVoidTy(context),
			{ i64, i8, i8 },
			false
		);

		llvm::Function* fn = llvm::Function::Create(
			fn_type,
			llvm::Function::PrivateLinkage,
			"__ec_log_data",
			module
		);
		
		fn->addFnAttr(llvm::Attribute::NoInline);

		llvm::Argument* arg_chunk = fn->getArg(0);
		llvm::Argument* arg_bl    = fn->getArg(1);
		llvm::Argument* arg_br    = fn->getArg(2);

		llvm::IRBuilder<> b(context);

		auto* blk = llvm::BasicBlock::Create(context, "entry", fn);
		b.SetInsertPoint(blk);

		// if i've counted right, 51 should be enough characters for a single row.
		// 2 (prefix) + 1 (space) + 16 (hex) + 1 (space) + 8 (ascii) + 1 (space) + log10(2^64) (decimal) + 1 (null terminator) = 50
		
		llvm::Value* buf = b.CreateAlloca(i8, b.getInt32(51), "buf");
		
		int pos = 0;

		auto store_valu = [&](llvm::Value* val, int at) {
			// get-element-pointer generates offset on pointer in platform-agnostic manner.	
			b.CreateStore(val, b.CreateGEP(i8, buf, b.getInt32(at)));
		};

		auto store_char = [&](char ch) {
			store_valu(b.getInt8(ch), pos++);
		};

		// 1. PREFIX

		store_valu(arg_bl, pos++);
		store_valu(arg_br, pos++);
		store_char(' ');

		// 2. HEX
		
		for (int nibble = 15; nibble >= 0; nibble--) { // most significnant first
			int shift_amt = nibble * 4;
			llvm::Value* shifted = b.CreateLShr(arg_chunk, b.getInt64(shift_amt));
			llvm::Value* masked = b.CreateAnd(shifted, b.getInt64(0xF)); // grab 4 bits at a times.
			llvm::Value* truncated = b.CreateTrunc(masked, i8); // dealing with 8 bit characters
			llvm::Value* decimal  = b.CreateAdd(truncated, b.getInt8('0'));
			llvm::Value* hex  = b.CreateAdd(truncated, b.getInt8('a' - 10));
			llvm::Value* is_less_than_ten = b.CreateICmpULT(truncated, b.getInt8(10));
			llvm::Value* hex_char = b.CreateSelect(is_less_than_ten, decimal, hex);
			store_valu(hex_char, pos++);
		}

		store_char(' ');

		// 3. ASCII
		
		for (int byte_idx = 7; byte_idx >= 0; byte_idx--) { // msb first
			int shift_amt = byte_idx * 8;
			llvm::Value* shifted = b.CreateLShr(arg_chunk, b.getInt64(shift_amt));
			llvm::Value* truncated = b.CreateTrunc(shifted, i8); // dealing with 8 bit characters, and grabbing 8 bits at a time.
			// printable range is generally recognized as 0x20 to 0x7e
			// other chars may do funny business: move cursor, play beep, etc , so are replaced with `.`
			llvm::Value* high_enough = b.CreateICmpUGE(truncated, b.getInt8(0x20));
			llvm::Value* low_enough = b.CreateICmpULE(truncated, b.getInt8(0x7e));
			llvm::Value* is_printable = b.CreateAnd(high_enough, low_enough);
			llvm::Value* ascii_char = b.CreateSelect(is_printable, truncated, b.getInt8('.'));
			store_valu(ascii_char, pos++);
		}

		store_char(' ');

		// 4. DECIMAL (unsigned only)
		
		int dec_tail = 48; // we are floating to the right and working our way to the left

		auto* block_dec_loop = llvm::BasicBlock::Create(context, "dec_loop", fn);
		auto* block_dec_done = llvm::BasicBlock::Create(context, "dec_done", fn);
		auto* block_fill_loop = llvm::BasicBlock::Create(context, "fill_loop", fn);
		auto* block_fill_done = llvm::BasicBlock::Create(context, "fill_done", fn);

		b.CreateStore(b.getInt8(0), b.CreateGEP(i8, buf, b.getInt32(dec_tail + 1))); // null terminator
		
		llvm::Value* alloca_idx = b.CreateAlloca(i32, nullptr, "idx");
		llvm::Value* alloca_dec_value = b.CreateAlloca(i64, nullptr, "dec_value");
		b.CreateStore(b.getInt32(dec_tail), alloca_idx);
		b.CreateStore(arg_chunk, alloca_dec_value);
		b.CreateBr(block_dec_loop);
		
		b.SetInsertPoint(block_dec_loop);
		llvm::Value* dec_value = b.CreateLoad(i64, alloca_dec_value);
		llvm::Value* digit_rem = b.CreateURem(dec_value, b.getInt64(10));
		llvm::Value* digit_truncated = b.CreateTrunc(digit_rem, i8);
		llvm::Value* digit_char = b.CreateAdd(digit_truncated, b.getInt8('0'));
		llvm::Value* idx = b.CreateLoad(i32, alloca_idx);
		b.CreateStore(digit_char, b.CreateGEP(i8, buf, idx));
		b.CreateStore(b.CreateSub(idx, b.getInt32(1)), alloca_idx);
		llvm::Value* dec_value_next = b.CreateUDiv(dec_value, b.getInt64(10));
		b.CreateStore(dec_value_next, alloca_dec_value);
		b.CreateCondBr(b.CreateICmpNE(dec_value_next, b.getInt64(0)), block_dec_loop, block_dec_done);
		
		b.SetInsertPoint(block_dec_done);
		b.CreateBr(block_fill_loop);

		b.SetInsertPoint(block_fill_loop);
		idx = b.CreateLoad(i32, alloca_idx);
		b.CreateStore(b.getInt8(' '), b.CreateGEP(i8, buf, idx));
		b.CreateStore(b.CreateSub(idx, b.getInt32(1)), alloca_idx);
		b.CreateCondBr(b.CreateICmpSGE(b.CreateLoad(i32, alloca_idx), b.getInt32(pos)), block_fill_loop, block_fill_done);

		b.SetInsertPoint(block_fill_done);
		b.CreateCall(puts_func, { buf });
		b.CreateRetVoid();

		return fn;
	}();

	// Takes in (i8* ptr, i64 byte_count).
	// byte_count of -1 means null-terminated.
	llvm::Function* log_data_deref_func = [&]() -> llvm::Function* {
		llvm::Type* i1 = llvm::Type::getInt1Ty(context);
		llvm::Type* i8 = llvm::Type::getInt8Ty(context);
		llvm::Type* i64 = llvm::Type::getInt64Ty(context);
		llvm::Type* i8ptr = llvm::Type::getInt8PtrTy(context);

		llvm::FunctionType* fn_type = llvm::FunctionType::get(
			llvm::Type::getVoidTy(context),
			{ i8ptr, i64 },
			false
		);

		llvm::Function* fn = llvm::Function::Create(
			fn_type,
			llvm::Function::PrivateLinkage,
			"__ec_log_data_deref",
			module
		);

		fn->addFnAttr(llvm::Attribute::NoInline);

		llvm::Argument* arg_ptr = fn->getArg(0);
		llvm::Argument* arg_byte_count = fn->getArg(1);

		llvm::IRBuilder<> b(context);
		
		// uses an outer loop for blocks and inner loop to deal with potential partial blocks.
		// because of "[---]", we have to implement lookahead so keep track of both chunk and chunk_prev. 

		auto* block_entry = llvm::BasicBlock::Create(context, "entry", fn);
		auto* block_outer_loop = llvm::BasicBlock::Create(context, "outer_loop", fn);
		auto* block_inner_setup = llvm::BasicBlock::Create(context, "inner_setup", fn);
		auto* block_inner_loop = llvm::BasicBlock::Create(context, "inner_loop", fn);
		auto* block_inner_body = llvm::BasicBlock::Create(context, "inner_body", fn);
		auto* block_inner_body3 = llvm::BasicBlock::Create(context, "inner_body3", fn);
		auto* block_inner_body2 = llvm::BasicBlock::Create(context, "inner_body2", fn);
		auto* block_render = llvm::BasicBlock::Create(context, "render", fn);
		auto* block_advance2 = llvm::BasicBlock::Create(context, "advance2", fn);
		auto* block_advance3 = llvm::BasicBlock::Create(context, "advance3", fn);
		auto* block_flush = llvm::BasicBlock::Create(context, "flush", fn);
		auto* block_flush_final = llvm::BasicBlock::Create(context, "flush_final", fn);
		auto* block_done = llvm::BasicBlock::Create(context, "done", fn);

		b.SetInsertPoint(block_entry);
		llvm::Value* alloca_outer_idx = b.CreateAlloca(i64, nullptr, "outer_idx");
		llvm::Value* alloca_inner_idx = b.CreateAlloca(i64, nullptr, "inner_idx");
		llvm::Value* alloca_chunk = b.CreateAlloca(i64, nullptr, "chunk");
		llvm::Value* alloca_chunk_prev = b.CreateAlloca(i64, nullptr, "chunk_prev");
		llvm::Value* alloca_has_prev = b.CreateAlloca(i1, nullptr, "has_prev");
		llvm::Value* alloca_is_first = b.CreateAlloca(i1, nullptr, "is_first");
		llvm::Value* alloca_is_game_over = b.CreateAlloca(i1, nullptr, "is_game_over");
		llvm::Value* is_nullterm = b.CreateICmpEQ(arg_byte_count, b.getInt64(-1), "is_nullterm");
		b.CreateStore(b.getInt64(0), alloca_outer_idx);
		b.CreateStore(b.getFalse(), alloca_has_prev);
		b.CreateStore(b.getFalse(), alloca_is_game_over);
		b.CreateBr(block_outer_loop);

		b.SetInsertPoint(block_outer_loop);
		llvm::Value* outer_idx = b.CreateLoad(i64, alloca_outer_idx);
		b.CreateCondBr(b.CreateLoad(i1, alloca_is_game_over), block_flush, block_inner_setup); // i feel like this won't handle zero rows properly.

		b.SetInsertPoint(block_inner_setup);
		b.CreateStore(b.getInt64(0), alloca_chunk);
		b.CreateStore(b.getInt64(0), alloca_inner_idx);
		b.CreateBr(block_inner_loop);

		b.SetInsertPoint(block_inner_loop);
		llvm::Value* inner_idx = b.CreateLoad(i64, alloca_inner_idx);
		llvm::Value* byte_idx = b.CreateAdd(outer_idx, inner_idx);
		llvm::Value* is_inner_done = b.CreateICmpEQ(inner_idx, b.getInt64(8));
		b.CreateCondBr(is_inner_done, block_render, block_inner_body);

		b.SetInsertPoint(block_inner_body);
		llvm::Value* byte_ptr = b.CreateGEP(i8, arg_ptr, byte_idx);
		llvm::Value* byte_value = b.CreateLoad(i8, byte_ptr);
		llvm::Value* is_null = b.CreateAnd(is_nullterm, b.CreateICmpEQ(byte_value, b.getInt8(0)));
		llvm::Value* is_ended = b.CreateAnd(b.CreateNot(is_nullterm), b.CreateICmpEQ(b.CreateAdd(byte_idx, b.getInt64(1)), arg_byte_count));
		llvm::Value* is_game_over = b.CreateOr(is_null, is_ended);
		b.CreateStore(is_game_over, alloca_is_game_over);
		llvm::Value* is_empty_chunk = b.CreateICmpEQ(inner_idx, b.getInt64(0));
		b.CreateCondBr(is_null, block_inner_body3, block_inner_body2);

		b.SetInsertPoint(block_inner_body3);
		b.CreateCondBr(is_empty_chunk, block_flush, block_render);

		b.SetInsertPoint(block_inner_body2);
		llvm::Value* extended = b.CreateZExt(byte_value, i64);
		llvm::Value* shift_amt = b.CreateSub(b.getInt64(56), b.CreateMul(inner_idx, b.getInt64(8))); // start from most significant side
		llvm::Value* chunk_original = b.CreateLoad(i64, alloca_chunk);
		b.CreateStore(b.CreateOr(chunk_original, b.CreateShl(extended, shift_amt)), alloca_chunk);
		b.CreateStore(b.CreateAdd(inner_idx, b.getInt64(1)), alloca_inner_idx);
		b.CreateCondBr(is_game_over, block_render, block_inner_loop);

		b.SetInsertPoint(block_render);
		llvm::Value* chunk = b.CreateLoad(i64, alloca_chunk);
		llvm::Value* has_prev = b.CreateLoad(i1, alloca_has_prev);
		b.CreateCondBr(has_prev, block_advance2, block_advance3);

		b.SetInsertPoint(block_advance2);
		llvm::Value* chunk_prev = b.CreateLoad(i64, alloca_chunk_prev);
		llvm::Value* is_first = b.CreateLoad(i1, alloca_is_first);
		llvm::Value* bl_of_interest = b.CreateSelect(is_first, b.getInt8('['), b.getInt8('-'));
		b.CreateCall(log_data_func, { chunk_prev, bl_of_interest, b.getInt8('-') });
		b.CreateBr(block_advance3);

		b.SetInsertPoint(block_advance3);
		llvm::Value* was_first = b.CreateNot(b.CreateLoad(i1, alloca_has_prev));
		b.CreateStore(chunk, alloca_chunk_prev);
		b.CreateStore(was_first, alloca_is_first);
		b.CreateStore(b.getTrue(), alloca_has_prev);
		b.CreateStore(b.CreateAdd(outer_idx, b.getInt64(8)), alloca_outer_idx);
		b.CreateBr(block_outer_loop);

		b.SetInsertPoint(block_flush);
		b.CreateCondBr(b.CreateLoad(i1, alloca_has_prev), block_flush_final, block_done);

		b.SetInsertPoint(block_flush_final);
		llvm::Value* chunk_final = b.CreateLoad(i64, alloca_chunk_prev);
		b.CreateCall(log_data_func, { chunk_final, b.CreateSelect(b.CreateLoad(i1, alloca_is_first), b.getInt8('['), b.getInt8('-')), b.getInt8(']') });
		b.CreateBr(block_done);

		b.SetInsertPoint(block_done);
		b.CreateRetVoid();

		return fn;
	}();

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
		
		std::shared_ptr<ValueSymbolTable> value_table = std::make_shared<ValueSymbolTable>(create_value_symbol_table());
		
		IrGenCtx igc = {
			context,
			module,
			builder,
			symbol_table,
			value_table,
			puts_func,
			log_data_func,
			log_data_deref_func,
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

static void populate_type_symbol_table(const json& structure, TypeSymbolTable& symbol_table) {
	for (const auto& entry : structure) {
		if (!entry.contains("type") || entry["type"] != "type") {
			continue;
		}
		
		if (!entry.contains("trail")) {
			fprintf(stderr, "Missing .trail\n");
			exit(1);
		}

		if (!entry.contains("definition")) {
			fprintf(stderr, "Missing .definition\n");
			exit(1);
		}

		std::string trail = entry["trail"].get<std::string>();
		Type normalized = normalize_type(entry["definition"], symbol_table);
		symbol_table.set(trail, promote_to_underlying(normalized));
	}
}

int main(int argc, char* argv[]) {
	// disable out-of-order prints by just disabling buffering.
	setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
	
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

	if (!parse_output.contains("structure") || !parse_output["structure"].is_array()) {
		fprintf(stderr, "There is no .structure\n");
		exit(1);
	}

	populate_type_symbol_table(parse_output["structure"], symbol_table);
	
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
