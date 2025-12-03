import { Command } from "commander";
import { writeFile } from "fs/promises";
import { join } from "path";
import { process as frontend_process } from "./frontend/process.js";
import { spawn } from "child_process";
import { create_promise } from "./frontend/uoe/create_promise.js";
import { throw_error } from "./frontend/uoe/throw_error.js";
import { error_internal } from "./frontend/uoe/error_internal.js";

const program = new Command();

const backend_process = async (result) => {
	console.log("Launching backend...");

	const [promise, resolve, reject] = create_promise();

	const backend_dir = join(import.meta.dirname, "backend");
	const args = [`${backend_dir}/host/run.sh`, result.module_path];

	const backend = spawn("bash", args, {
		stdio: "inherit",
		cwd: backend_dir,
	});

	backend.on("close", (code) => {
		if (code === 0) {
			resolve();
		} else {
			reject(throw_error(error_internal(`Backend exited with unsatisfactory code ${code}`)));
		}
	});

	backend.on("error", (err) => {
		reject(err);
	});

	return promise;
};

const compile_module = async (config) => {
	console.log("Compiling module located at:", config.src);
	const result = await frontend_process(config);
	const out_path = join(result.module_path, "frontend.out.json");

	const safe_result = {
		...result,
		parse_output: {
			...result.parse_output,
			source_scopes: undefined,
		},
	};

	await writeFile(out_path, JSON.stringify(safe_result, (_key, value) => {
		if (typeof value === "bigint") {
			return value.toString();
		}

		return value;
	}, "\t"));

	console.log("Frontend output dumped to", out_path);

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
