import fs from "fs/promises";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const root_path = path.resolve(__dirname, "../../");
const output_path = path.join(__dirname, "appendix_source_code.tex");

const ignored_paths = new Set([
	"uoe",
	"pnpm-lock.yaml",
	".gitignore",
	".vscodeignore",
]);

const ignored_extensions = new Set([
	".pdf",
	".png",
	".tex",
]);

const escape_latex = (text) => {
	return (text
		.replace(/\\/g, "\\textbackslash{}")
		.replace(/\^/g, "\\textasciicircum{}")
		.replace(/~/g, "\\textasciitilde{}")
		.replace(/[{}&%$#_]/g, "\\$&")
	);
};

const escape_verbatim = (text) => {
	// don't ask me how many hours it took to figure this out

	return (text
		.replace(/@/g, "@\\char\"40@")
		.replace(/\\end\{lstlisting\}/g, "@\\textbackslash{}end\{lstlisting\}@")
		// .replace(/\\/g, "(*@\\textbackslash{}@*)")
		// .replace(/%/g, "(*@\\%@*)");
	);
};

const parse_gitignore = async (path) => {
	const patterns = [];

	try {
		var content = await fs.readFile(path, "utf8");
	} catch (err) {
		console.log("No .gitignore found at:", path);
		return [];
	}

	const lines = content.split("\n");

	for (let line of lines) {
		line = line.trim();

		if ((false
			|| line === ""
			|| line.startsWith("#")
		)) {
			continue;
		}

		const is_negated = line.startsWith("!");

		if (is_negated) {
			line = line.substring(1);
		}

		patterns.push({ pattern: line, is_negated });
	}

	return patterns;
};

const pattern_to_regex = (pattern) => {
	const is_root = pattern.startsWith("/");

	if (is_root) {
		pattern = pattern.substring(1);
	}

	const is_directory_only = pattern.endsWith("/");

	if (is_directory_only) {
		pattern = pattern.substring(0, pattern.length - 1);
	}

	let regex_str = "";

	for (let i = 0; i < pattern.length; i++) {
		const char = pattern[i];

		if (char === "*") {
			if ((true
				&& i + 1 < pattern.length
				&& pattern[i + 1] === "*"
			)) {
				regex_str += ".*";
				i++;
				if ((true
					&& i + 1 < pattern.length
					&& pattern[i + 1] === "/"
				)) {
					i++;
				}
			} else {
				regex_str += "[^/]*";
			}
		} else if (char === "?") {
			regex_str += "[^/]";
		} else if ((false
			|| ".*+?^${}()|[]\\".includes(char)
		)) {
			regex_str += "\\" + char;
		} else {
			regex_str += char;
		}
	}

	if (is_root) {
		regex_str = "^" + regex_str;
	} else {
		regex_str = "(^|/)" + regex_str;
	}

	if (is_directory_only) {
		regex_str += "(/|$)";
	} else {
		regex_str += "(/.*|$)";
	}

	return new RegExp(regex_str);
};

const load_gitignore_rules = async (base_dir) => {
	const rules_by_dir = new Map();

	const analyze_directory = async (dir, rel_path) => {
		rel_path ??= "";

		const gitignore_path = path.join(dir, ".gitignore");
		const patterns = await parse_gitignore(gitignore_path);

		if (patterns.length > 0) {
			const normalized_rel_path = rel_path.replace(/\\/g, "/");
			rules_by_dir.set(normalized_rel_path, patterns);
		}

		const entries = await fs.readdir(dir, { withFileTypes: true });

		for (const entry of entries) {
			if ((true
				&& entry.isDirectory()
				&& entry.name !== ".git"
			)) {
				const new_rel_path = rel_path ? path.join(rel_path, entry.name) : entry.name;
				await analyze_directory(path.join(dir, entry.name), new_rel_path);
			}
		}
	};

	await analyze_directory(base_dir);

	return rules_by_dir;
};

const is_ignored = (file_path, _is_directory, rules_by_dir) => {
	const normalized = file_path.replace(/\\/g, "/");
	const segments = normalized.split("/");

	let is_ignored = false;

	for (let i = 0; i <= segments.length; i++) {
		const dir_path = segments.slice(0, i).join("/");
		const remaining = segments.slice(i).join("/");

		const rules = rules_by_dir.get(dir_path === "" ? "" : dir_path) ?? [];

		for (const { pattern, is_negated } of rules) {
			const regex = pattern_to_regex(pattern);

			if (regex.test(remaining)) {
				is_ignored = !is_negated;
			}
		}
	}

	if ((false
		|| normalized === ".git"
		|| normalized.startsWith(".git/")
	)) {
		is_ignored = true;
	}

	for (const part of segments) {
		if (ignored_paths.has(part)) {
			is_ignored = true;
			break;
		}
	}

	for (const ext of ignored_extensions) {
		if (normalized.endsWith(ext)) {
			is_ignored = true;
			break;
		}
	}

	return is_ignored;
};

const collect_files = async (dir, base_path, rules_by_dir) => {
	const results = [];
	const entries = await fs.readdir(dir, { withFileTypes: true });

	for (const entry of entries) {
		const full_path = path.join(dir, entry.name);
		const rel_path = base_path ? path.join(base_path, entry.name) : entry.name;

		if (is_ignored(rel_path, entry.isDirectory(), rules_by_dir)) {
			continue;
		}

		if (entry.isDirectory()) {
			results.push(...await collect_files(full_path, rel_path, rules_by_dir));
		} else if (entry.isFile()) {
			results.push(rel_path);
		}
	}

	return results;
};

const generate_latex = async () => {
	console.log("Collecting all files from:", root_path);
	console.log("Loading .gitignore rules...");

	const rules_by_dir = await load_gitignore_rules(root_path);
	console.log(`Loaded .gitignore rules from ${rules_by_dir.size} directories.`);

	const files = await collect_files(root_path, "", rules_by_dir);
	files.sort();

	console.log(`Found ${files.length} files to include in this report's appendix.`);

	let latex = "";
	latex += "% This is an auto-generated appendix using my appendix generator script\n";
	latex += "% Date of generation: " + new Date().toISOString() + "\n\n";
	latex += "\\lstset{\n";
	latex += "  breaklines=true,\n";
	latex += "  breakatwhitespace=false,\n";
	latex += "  postbreak={\\mbox{\\textcolor{red}{$\\hookrightarrow$}\\space}},\n";
	latex += "  columns=fixed,\n";
	latex += "  keepspaces=true,\n";
	latex += "  showstringspaces=false,\n";
	latex += "  tabsize=4,\n";
	latex += "}\n\n";

	let i = 0;

	for (const file_path of files) {
		console.log(`Processing: ${file_path}`);

		const full_path = path.join(root_path, file_path);

		const content = await fs.readFile(full_path, "utf8");

		const normalized_path = file_path.replace(/\\/g, "/");
		const escaped_path = escape_latex(normalized_path);
		const escaped_content = escape_verbatim(content);

		latex += "\\vspace{0.5cm}\n";
		latex += "\\noindent\\colorbox{lightgray}{\\parbox{\\dimexpr\\textwidth-2\\fboxsep}{\\small\\textbf{" + escaped_path + "}}}\n\n";
		latex += "\\vspace{0.2cm}\n\n";
		latex += "\\begin{lstlisting}[language={}, escapechar=@, basicstyle=\\ttfamily\\footnotesize, breaklines=true, breakatwhitespace=false]\n";

		// change to limit how many files are outputted for debugging purposes.
		if (i <= 999) { // 17
			latex += escaped_content;
		}

		if (!escaped_content.endsWith("\n")) {
			latex += "\n";
		}

		latex += "\\end{lstlisting}\n\n";

		i++;
	}

	await fs.writeFile(output_path, latex, "utf8");
	console.log(`\nLatex appendix has been succesfully outputted to: ${output_path}`);
	console.log(`Total number of files: ${files.length}`);
};

generate_latex();
