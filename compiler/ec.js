import { Command } from "commander";
import { process as frontend_process } from "./frontend/process.js";

const program = new Command();

const backend_process = () => { };

const compile_module = async (config) => {
	console.log("Compiling module located at:", config.src);
	const result = await frontend_process(config);
	console.dir(result, { depth: null });
	await backend_process(result);
};

program
	.name("ec")
	.description("Essence C compiler")
	.version("1.0.0");

program
	.command("compile <src> <dest>")
	.description("Compile a module located at <src> and output build artifacts to <dest>")
	.action(async (src, dest, _options) => {
		await compile_module({
			src,
			dest,
		});
	});

program.parse();
