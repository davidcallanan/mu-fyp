import fs from "fs/promises";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const DEBUG_QUICK_TESTING = false;
const DEBUG_PRINT_IDS = false;

const input_path = path.join(__dirname, "input.json");
const output_path = path.join(__dirname, "appendix_llm_prompts.tex");

// this allows me to filter FYP-related threads from all my personal threads on the same account.
const thread_filter = [
	{ tid: "7add1779-4337-44f1-a533-c37c4eb71efe" }, // done
	{ tid: "765f7232-673f-4bf1-a4e4-a927957dbf00" }, // done
	{ tid: "054b94b1-6432-4f9c-bcb6-42b0e0b916c5" }, // done
	{
		tid: "28da42c3-1ae9-4987-b23e-0595750ca2e5",
		mids: [ // done
			"j977xk3e5cmtr0k2w9emv2qdm17hy3dv",
			"j972dx1905kmscg1nn7b3xd9rx7hy2dd",
		]
	},
	{ tid: "6deac944-2edd-4644-9955-c8a780772eb2" }, // done
	{ tid: "a9183424-da00-4790-9489-d0f65cddffef" }, // done
	{ tid: "f97198a6-f399-4912-a023-8740dc2583dd" }, // done
	{ tid: "42e8ae10-5d13-4ecb-8821-841f9ec04e0e" }, // done
	{ tid: "84682898-db2e-4b16-83d1-a793f25c8ed0" }, // done
];

// latex is struggling with unicode - for now just gonna remove a bunch of it.
const wipe_unicode = (text) => {
	return text.replace(/[^\x00-\x7F]/g, "");
};

const escape_latex = (text) => {
	const clean_text = wipe_unicode(text);

	return (clean_text
		.replace(/\\/g, "\\textbackslash{}")
		.replace(/\^/g, "\\textasciicircum{}")
		.replace(/~/g, "\\textasciitilde{}")
		.replace(/[{}&%$#_]/g, "\\$&")
	);
};

const escape_verbatim = (text) => {
	const clean_text = wipe_unicode(text);

	return (clean_text
		.replace(/@/g, "@\\char\"40@")
		.replace(/\\end\{lstlisting\}/g, "@\\textbackslash{}end\{lstlisting\}@")
	);
};

const format_date_nicely = (iso_string) => {
	const date = new Date(iso_string);
	const year = date.getFullYear();
	const month = String(date.getMonth() + 1).padStart(2, "0");
	const day = String(date.getDate()).padStart(2, "0");
	const hours = String(date.getHours()).padStart(2, "0");
	const minutes = String(date.getMinutes()).padStart(2, "0");

	return `${year}/${month}/${day} ${hours}:${minutes}`;
};

const decide_message_color = (message) => {
	const role = message.role ?? "";

	if (role === "user") {
		return "blue!15";
	}

	if (role === "assistant") {
		return "red!15";
	}

	return "gray!10";
};

const should_include_message = (thread_id, message_id) => {
	const filter_entry = thread_filter.find((entry) => entry.tid === thread_id);

	if (!filter_entry) {
		return false;
	}

	if (!filter_entry.mids) {
		return true;
	}

	return filter_entry.mids.includes(message_id);
};

const generate_latex = async () => {
	console.log("Reading input from file:", input_path);

	const content = await fs.readFile(input_path, "utf8");

	const input_data = JSON.parse(content);

	console.log("top level keys", Object.keys(input_data).join(", "));

	const filtered_threads = [];

	for (const thread of (input_data.threads ?? []).reverse()) {
		const filter_entry = thread_filter.find((entry) => entry.tid === thread.id);

		if (!filter_entry) {
			continue;
		}

		console.log(`Found desired thread: ${thread.id}`);

		let filtered_messages = [];

		const messages_in_thread = (input_data.messages ?? []).filter((msg) => msg.threadId === thread.id);
		console.log(`thread has ${messages_in_thread.length} messages.`);

		for (const message of messages_in_thread) {
			if (should_include_message(thread.id, message.id)) {
				filtered_messages.push(message);
			}
		}

		if (filtered_messages.length > 0) {
			if (DEBUG_QUICK_TESTING) { // just chop off any more than 2 messages to speed up latex compilation
				filtered_messages = filtered_messages.slice(0, 2);
			}

			filtered_threads.push({
				...thread,
				messages: filtered_messages,
			});
		}
	}

	console.log(`Filtered data to ${filtered_threads.length} relevant threads.`);

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

	let message_count = 0;

	for (const thread of filtered_threads) {
		let message_number = 1;

		const thread_start_time = format_date_nicely(thread.created_at);

		latex += "\\vspace{0.5cm}\n";
		latex += "\\noindent\\colorbox{lightgray}{\\parbox{\\dimexpr\\textwidth-2\\fboxsep}{\\small\\textbf{Thread (" + thread.messages.length + " messages) \\newline ID: " + thread.id;

		if (DEBUG_PRINT_IDS) {
			latex += " | ID: " + thread.id;
		}

		latex += "\\hfill " + thread_start_time + "}}}\n\n";
		latex += "\\vspace{0.2cm}\n\n";

		for (const message of thread.messages) {
			const message_date_time = format_date_nicely(message.created_at);
			const escaped_content = escape_verbatim(message.content || "");
			const message_color = decide_message_color(message);
			const role = message.role.toLowerCase();
			const role_display = role === "user" ? "USER" : "AGENT";
			const model_name = escape_latex(message.model ?? "unknown-model");

			latex += "\\noindent\\colorbox{" + message_color + "}{\\parbox{\\dimexpr\\textwidth-2\\fboxsep}{\\small \\textbf{" + role_display + "} (Message " + message_number + ") [" + model_name + "]";

			if (DEBUG_PRINT_IDS) {
				latex += " | ID: " + message.id;
			}

			latex += "\\hfill " + message_date_time + "}}\n\n";
			latex += "\\vspace{0.2cm}\n\n";
			latex += "\\begin{lstlisting}[language={}, escapechar=@, basicstyle=\\ttfamily\\footnotesize, breaklines=true, breakatwhitespace=false]\n";
			latex += escaped_content;

			if (!escaped_content.endsWith("\n")) {
				latex += "\n";
			}

			latex += "\\end{lstlisting}\n\n";

			message_count++;
			message_number++;
		}
	}

	await fs.writeFile(output_path, latex, "utf8");
	console.log(`\nLatex appendix has been successfully dumped to: ${output_path}`);
	console.log(`Total number of threads is ${filtered_threads.length}`);
	console.log(`Total number of messages is ${message_count}`);
};

generate_latex();
