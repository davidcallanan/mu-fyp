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
	// after interim report:
	{ tid: "e2ee05ae-9d50-458c-99a1-c2d42bad846b" }, // done
{ tid: "8c5ec010-e001-42c8-bbfc-0300d6e5aa06" }, // done
{ tid: "ccb91c1b-5a84-41b2-83bf-8323c233fca0" }, // done
{ tid: "02eff1db-ddf4-450f-8e28-8fbee75286b5" }, // done
{ tid: "0f52e365-c720-4bf1-9366-564322f1d0f2" }, // done
{ tid: "1ca1acef-4d34-4ebb-82df-c4db8d94cd25" }, // done
{ tid: "6a2652b2-b673-4868-8f12-c657aa570ce6" }, // done
{ tid: "267dd077-068f-4ade-b974-797d55101633" }, // done
{ tid: "6b730afb-7c78-419c-9eae-9a91302d3036" }, // done
{ tid: "99f2d159-7621-41e9-82b4-c081113d9868" }, // done
{ tid: "8cf2a043-a387-40d9-98d4-aa384f7ee2ff" }, // done
{ tid: "58580f97-ce81-4716-94e0-edd2f8a8bc0b" }, // done
{ tid: "528af2d0-432b-4816-a92f-b19515ebac54" }, // done
{ tid: "1b58c5fb-26f3-4be7-8151-fdd376016c4e" }, // done
{ tid: "cf8b8fd1-fb36-40ce-99e4-7ca2d53f2e10" }, // done
{ tid: "0868c8c4-4203-4a18-b970-1358388bfc5c" }, // done
{ tid: "411d6cb2-dc62-4189-a5e0-7b00438aeee5" }, // done
{ tid: "16438041-c253-46d0-a304-0afb26c53067" }, // done
{ tid: "7013cb4f-d11c-41da-bf1e-51d842248d37" }, // done
{ tid: "e5e91bb7-fb2a-446e-8161-eff48cd7bc45" }, // done
{ tid: "7a34829c-564a-47e4-998a-6f52e79c1069" }, // done
{ tid: "a152cbf5-7462-4718-a9d3-503ccfbd676b" }, // done
{ tid: "65ad1518-f5c5-44d0-99e9-368b55f728f2" }, // done
{ tid: "00600734-2bef-4f9b-bbef-a2715c96627c" }, // done
{ tid: "0eb877a4-068c-4d12-8838-c981bb6c2e54" }, // done
{ tid: "41b51d71-f1f9-4f37-8694-6fd5e6061772" }, // done
{ tid: "d5b5ce94-19ca-48bb-a8b5-8867a22c82dc" }, // done
{ tid: "220bd1bc-d0d0-4623-b156-0ccdd13abd54" }, // done
{ tid: "5be22e41-c53b-4bbb-bdb0-ebfa83217df0" }, // done
{ tid: "8d9b8af7-d0f9-40c6-b08d-2c81f0e8ab68" }, // done
{ tid: "f63db2d0-dff6-4a41-8b45-c11e856d94e7" }, // done
{ tid: "8c303c5a-d9da-4986-afcd-d28fcf4da5ae" }, // done
{ tid: "9c0082fb-6742-4342-b697-f981645d0829" }, // done
{ tid: "2eb8c8fd-2dab-42c5-bdcc-b0011008a4cd" }, // done
{ tid: "fe5f388b-ccee-4009-ad3f-83921fd5aad7" }, // done
{ tid: "fc3b0c50-14f2-42a6-bf24-ba80cae92faa" }, // done
{ tid: "1b8c736a-c8bd-45a0-8b9c-36ea4859aad9" }, // done
{ tid: "d51b29af-e51e-4013-b954-53eddd1c0476" }, // done
{ tid: "d43d9c4b-d36a-4439-8997-c93ba8a48fc2" }, // done
{ tid: "a45edcfd-726b-4b9a-bc24-6eb2ca115265" }, // done
{ tid: "6856e3da-1ac3-4bb2-8711-2a97bc965ccc" }, // done
{ tid: "c3e3cd87-4623-4892-9897-377e8eb8f51b" },// done
{ tid: "297ae3e4-b3de-4ad2-bc7e-623b478ba9cd" },// done
{ tid: "3975119d-aa9d-44ca-b13b-167e059ab9e8" },// done
{ tid: "469a5465-2335-4028-b960-ee5d4d6a055b" },// done
{ tid: "bee9220b-d6e2-4ae9-87a0-9da11868a68c" },// done
{ tid: "9c200fd4-c9ff-42ad-92c0-b2878229a845" },// done
{ tid: "2b566c00-e8b3-4461-8717-81c661f45e8a" },// done
{ tid: "3cf7f0e9-b870-4b6d-aa65-369a98cb09da" },// done
{ tid: "e78a70d5-6685-4df2-beee-f027c5cd2d79" },// done
{ tid: "22f347ad-f89e-4ccb-af05-6f98cf2923a5" },// done
{ tid: "e4e0e8e6-c00e-4b99-9333-6a022f841626" },// done
{ tid: "895ce06a-dbf5-4ee5-b393-1b280bb9d237" },// done
{ tid: "1998cab0-d0dd-4371-a417-51e8bb406bbf" },// done
{ tid: "e7dd9610-2154-4ada-a4e6-f52299f357cd" },// done
{ tid: "30b77145-ec01-4eb7-8d0f-0fe98c58c1fa" },// done
{ tid: "d16d0560-c104-4e2e-a800-6c2be3280db3" },// done
{ tid: "732205cb-ffe3-4d67-b653-ddb7c0e6dd10" },// done
{ tid: "3081a268-2ec9-475b-9ac5-a3854793dca8" },// done
{ tid: "5f5fdb89-f016-4400-a1d6-76f76c2b1fe9" },// done
{ tid: "b3ca97ba-f502-4fbc-8dcc-9961df2adb6d" },// done
{ tid: "d64a12cf-b4a0-4924-b169-81c99ef07dcc" },// done
{ tid: "6e39b0a0-a352-4bb6-a4ac-c37c756f76ac" },// done
{ tid: "281b3195-7e0d-40b7-a596-732b75ebb47e" },// done
{ tid: "1dd68212-43f2-4077-a5d1-91779b17e061" },// done
{ tid: "a798a4dd-34ee-4c64-ae3a-4d971af7f7dd" },// done
{ tid: "b64a218f-87c7-4cec-99d4-53203bb5af00" },// done
{ tid: "c1506474-ee5e-4716-8318-e678906ea414" },// done
{ tid: "d2818821-e2c8-48f4-8da2-b393c7e67c15" },// done
{ tid: "63fdf157-a1e5-45c1-b2a8-aaa69fa9b13b" },// done
{ tid: "8a336fa4-724e-4864-aa53-0da65ba153c2" },// done
{ tid: "c1db4b8f-2baf-4e5e-b113-eecd8f19c9ed" },// done
{ tid: "180eca74-06e2-4ccc-abce-e47bcd8a9b0e" },// done
{ tid: "4fb34f3a-a64e-4271-b075-5f630ca2ca05" },// done
{ tid: "44a2ec6a-0e5d-479a-bcef-f0c9b4e1a8bc" },// done
{ tid: "c4bcd071-f9dc-4808-bfcc-e235212260c9" },// done
{ tid: "50510658-0972-4b7b-a2d0-133df3ce22e4" },// done
{ tid: "37a7b418-b307-4ec5-90a2-97c9743ba110" },// done
{ tid: "681be10a-5a74-4e19-9f69-b69051c76e0f" },// done
{ tid: "491420bd-b00a-42c4-a9fe-4a894b423ab8" },// done
{ tid: "1c857b18-72ea-4436-96fd-d06d20bde43a" },// done
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
			const basic_style = role === "assistant" ? "\\ttfamily\\tiny" : "\\ttfamily\\footnotesize";

			latex += "\\noindent\\colorbox{" + message_color + "}{\\parbox{\\dimexpr\\textwidth-2\\fboxsep}{\\small \\textbf{" + role_display + "} (Message " + message_number + ") [" + model_name + "]";

			if (DEBUG_PRINT_IDS) {
				latex += " | ID: " + message.id;
			}

			latex += "\\hfill " + message_date_time + "}}\n\n";
			latex += "\\vspace{0.2cm}\n\n";
			latex += "\\begin{lstlisting}[language={}, escapechar=@, basicstyle=" + basic_style + ", breaklines=true, breakatwhitespace=false]\n";
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
