import { mapData, join, opt, multi, opt_multi, or, declare, trace_print, trace_dump, rule } from "./uoe/ec/blurp.js";

import * as fs from "fs/promises";
import * as node__path from "path";
import { randomUUID } from "crypto";
import { create_fs_source_scope } from "./create_fs_source_scope.js";
import { create_source_scope as orig_create_source_scope } from "./create_source_scope.js";
import { create_source_extractor } from "./create_source_extractor.js";
import { create_dir_node_translator } from "./create_dir_node_translator.js";
import { throw_error } from "./uoe/throw_error.js";
import { error_internal } from "./uoe/error_internal.js";

// LEXICAL TOKENS

const WHITESPACE = rule("WHITESPACE", /^\s+/);
const REALSPACE = rule("REALSPACE", /^[ \t]+/);
const SINGLELINE_COMMENT = rule("SINGLELINE_COMMENT", mapData(/^\s*;(.*?)(\n|$)\s*/, data => data.groups[0]));
const SKIPPERS = rule("SKIPPERS", opt(multi(or(SINGLELINE_COMMENT, WHITESPACE))));
const CORE_SKIPPERS = rule("CORE_SKIPPERS", opt(multi(REALSPACE)));
const NEWLINE = rule("NEWLINE", /^\n/);

const withSkippers = (p) => mapData(join(SKIPPERS, p, SKIPPERS), data => data[1]);
const withCarefulSkippers = (p) => mapData(join(SKIPPERS, p, CORE_SKIPPERS), data => data[1]);
const withLeftSkippers = (p) => mapData(join(SKIPPERS, p), data => data[1]);
const withRightSkippers = (p) => mapData(join(p, SKIPPERS), data => data[0]);

const MANDATORY_NEWLINE = rule("MANDATORY_NEWLINE", join(CORE_SKIPPERS, or(SINGLELINE_COMMENT, NEWLINE)));

const KW_FORWARDING = rule("KW_FORWARDING", withCarefulSkippers("forwarding"));
const KW_RESET = rule("KW_RESET", withCarefulSkippers("reset"));
const KW_TYPE = rule("KW_TYPE", withCarefulSkippers("type"));
const KW_TYPES = rule("KW_TYPES", withCarefulSkippers("types"));
const KW_MAP = rule("KW_MAP", withCarefulSkippers("map"));
const KW_IMPORT = rule("KW_IMPORT", withCarefulSkippers("import"));
const KW_MOD = rule("KW_MOD", withCarefulSkippers("mod"));
const KW_CREATE = rule("KW_CREATE", withCarefulSkippers("create"));
const KW_LOG = rule("KW_LOG", withCarefulSkippers("log"));
const LBRACE = rule("LBRACE", withCarefulSkippers("{"));
const RBRACE = rule("RBRACE", withCarefulSkippers("}"));
const LPAREN = rule("LPAREN", withCarefulSkippers("("));
const RPAREN = rule("RPAREN", withCarefulSkippers(")"));
const ARROW = rule("ARROW", withCarefulSkippers("->"));
const ASTERISK = rule("ASTERISK", withCarefulSkippers("*"));
const COMMA = rule("COMMA", withCarefulSkippers(","));
const PATH_OUTER_ROOT = rule("PATH_OUTER_ROOT", withCarefulSkippers(mapData(/^\/\/[a-z0-9_\/\.]*/, data => data.groups.all)));
const PATH_MODULE_ROOT = rule("PATH_MODULE_ROOT", withCarefulSkippers(mapData(/^\/[a-z0-9_\/\.]*/, data => data.groups.all)));
const PATH = rule("PATH", or(PATH_OUTER_ROOT, PATH_MODULE_ROOT));
const OUTER_ROOT = rule("OUTER_ROOT", withCarefulSkippers("//"));
const SEMI = rule("SEMI", withRightSkippers(SINGLELINE_COMMENT));
// i need to go through all these again. why did I put "_" outside the []. why is underscore and numbers not matched in the first part?
const TYPE_IDENT = rule("TYPE_IDENT", withCarefulSkippers(mapData(/^(?:(?:([a-z]+(?:_[a-z0-9]+)*::)*[A-Z][a-zA-Z0-9]*)|i[1-9][0-9]{0,4}|u[1-9][0-9]{0,4}|f16|f32|f64|f128)/, data => data.groups.all)));
const TYPES_IDENT = rule("TYPES_IDENT", withCarefulSkippers(mapData(/^(?:([a-z]+(?:_[a-z0-9]+)*::)*(?:[a-z]+(?:_[a-z0-9]+)*))/, data => data.groups.all)));
const MOD_IDENT = rule("MOD_IDENT", withCarefulSkippers(mapData(/^(?:([a-z]+(?:_[a-z0-9]+)*::)*(?:[a-z]+(?:_[a-z0-9]+)*))/, data => data.groups.all))); // identical to TYPES_IDENT for now.
const SYMBOL = rule("SYMBOL", withCarefulSkippers(mapData(/^:([a-zA-Z_][a-zA-Z0-9_]*)/, data => data.groups.all)));
const IDENT = rule("IDENT", withCarefulSkippers(mapData(/^([a-zA-Z_][a-zA-Z0-9_]*)/, data => data.groups.all)));
const WALRUS = rule("WALRUS", withCarefulSkippers(":="));
const EXTAT = rule("EXTAT", withLeftSkippers("@"));
const INTEGER = rule("INTEGER", withCarefulSkippers(mapData(/^[0-9]+/, data => BigInt(data.groups.all))));
const FLOAT = rule("FLOAT", withCarefulSkippers(mapData(/^[0-9]+\.[0-9]+/, data => data.groups.all)));
const STRING = rule("STRING", withCarefulSkippers(mapData(/^"((?:[^"\\\r\n]|\\.)*)"/, data => data.groups[0])));

// PARSER RULES

const symbol_path = rule("symbol_path", mapData(
	multi(
		SYMBOL,
	),
	(data) => ({
		type: "symbol_path",
		trail: data.map(entry => entry.substring(1)),
	}),
));

const map_entry_log = rule("map_entry_log", mapData(
	join(KW_LOG, LPAREN, opt(STRING), RPAREN),
	(data) => ({
		type: "map_entry_log",
		message: data[2],
	}),
));

const hardval = declare();
const typeval = declare();
const typeval_atom = declare();

hardval.define(rule("hardval", or(
	mapData(
		INTEGER,
		(data) => ({
			type: "type_map",
			leaf_type: undefined,
			leaf_hardval: {
				type: "hardval_integer",
				value: data,
			},
			call_input_type: undefined,
			call_output_type: undefined,
			sym_inputs: {},
			instructions: [],
		}),
	),
	mapData(
		FLOAT,
		(data) => ({
			type: "type_map",
			leaf_type: undefined,
			leaf_hardval: {
				type: "hardval_float",
				value: data,
			},
			call_input_type: undefined,
			call_output_type: undefined,
			sym_inputs: {},
			instructions: [],
		}),
	),
)));

const map_entry_assign = rule("map_entry_assign", mapData(
	join(IDENT, WALRUS, typeval),
	(data) => ({
		type: "map_entry_assign",
		name: data[0],
		typeval: data[2],
	}),
));

const map_entry = rule("map_entry", or(
	mapData(map_entry_log, (data) => ({
		type: "instruction",
		data,
	})),
	mapData(map_entry_assign, (data) => ({
		type: "instruction",
		data,
	})),
));

const constraint_map_braced_multiline = rule("constraint_map_braced_multiline", mapData(
	join(
		LBRACE,
		MANDATORY_NEWLINE,
		opt_multi(
			join(
				map_entry,
				SEMI,
			),
		),
		RBRACE,
	),
	(data) => {
		const entries = data[2].map(entry => entry[0]);
		
		const sym_inputs = Object.fromEntries(
			(entries
				.filter(entry => entry.type === "sym_input")
				.map(entry => [entry.name, entry.data])
			),
		);
		
		const instructions = (entries
			.filter(entry => entry.type === "instruction")
			.map(entry => entry.data)
		);
			
		return {
			type: "type_map",
			sym_inputs,
			instructions,
		}
	},
));

const constraint_map_braced_singleline = rule("constraint_map_braced_singleline", mapData(
	join(
		LBRACE,
		opt(
			join(
				map_entry,
				opt_multi(join(COMMA, map_entry)),
			),
		),
		RBRACE,
	),
	(data) => {
		const entries = [];

		if (data[1] !== undefined) {
			entries.push(data[1][0]);

			for (const entry of data[1][1]) {
				entries.push(entry[1]);
			}
		}
		
		const sym_inputs = Object.fromEntries(
			(entries
				.filter(entry => entry.type === "sym_input")
				.map(entry => [entry.name, entry.data])
			),
		);
		
		const instructions = (entries
			.filter(entry => entry.type === "instruction")
			.map(entry => entry.data)
		);
			

		return {
			type: "type_map",
			sym_inputs,
			instructions,
		};
	},
));

const constraint_map_braced = rule("constraint_map_braced", or(
	constraint_map_braced_multiline,
	constraint_map_braced_singleline,
));

const constraint_map_tupled = rule("constraint_map_tupled", mapData(
	join(
		LPAREN,
		RPAREN,
	),
	(data) => ({
		type: "type_map",
	}),
));

const constraint_map = rule("constraint_map", or(
	constraint_map_braced,
	constraint_map_tupled,
));

const constraint_integer = rule("constraint_integer", mapData(
	INTEGER,
	(data) => ({
		type: "constraint",
		mode: "constraint_integer",
		value: data,
	}),
));

const constraint_float = rule("constraint_float", mapData(
	FLOAT,
	(data) => ({
		type: "constraint",
		mode: "constraint_float",
		value: data,
	}),
));

const constraint_semiless = rule("constraint_semiless", or(
	constraint_map,
));

const constraint_semiful = rule("constraint_semiful", or(
	constraint_float,
	constraint_integer,
));

const constraint_maybesemi = rule("constraint_maybesemi", or(
	constraint_semiless,
	mapData(
		join(constraint_semiful, SEMI),
		(data) => data[0],
	),
));

const forwarding = rule("forwarding", mapData(
	join(
		KW_FORWARDING,
		LBRACE,
		opt_multi(
			or(
				mapData(
					// for now, only makes sense to reset a path in the outer root.
					join(KW_RESET, or(PATH_OUTER_ROOT, OUTER_ROOT), SEMI),
					(data) => ({ type: "reset", source: data[0] }),
				),
				mapData(
					join(PATH_OUTER_ROOT, PATH, SEMI),
					(data) => ({ type: "redirect", source: data[0], destination: data[1] })
				),
			)
		),
		RBRACE,
	),
	(data) => ({
		type: "forwarding",
		entries: data[2],
	}),
));

const top_forwarding = rule("top_forwarding", mapData(
	forwarding,
	(data) => ({
		...data,
		type: "top_forwarding",
	}),
));

const type_reference = declare();
// todo: probably just declare everything at the top to avoid stress.

type_reference.define(rule("type_reference", or(
	mapData(
		KW_MAP,
		(_data) => ({
			type: "type_reference",
			mode: "anon_map",
			leaf_type: undefined,
			call_input_type: undefined,
			call_output_type: undefined,
		}),
	),
	mapData(
		TYPE_IDENT,
		(data) => ({
			type: "type_reference",
			mode: "type_ident",
			trail: data.split("::"),
		}),
	),
	mapData(
		join(ASTERISK, type_reference),
		(data) => ({
			type: "type_reference",
			mode: "pointer",
			uly: data[1],
		}),
	),
)));

const type_named = rule("type_named", mapData(
	TYPE_IDENT,
	(data) => ({
		type: "type_named",
		trail: data,
	}),
));

typeval_atom.define(rule("typeval_atom", or(
	mapData(
		join(type_named, hardval),
		(data) => ({
			type: "type_map",
			leaf_type: data[0],
			leaf_hardval: data[1],
			call_input_type: undefined,
			call_output_type: undefined,
			sym_inputs: {},
			instructions: [],
		}),
	),
	mapData(
		join(type_named, constraint_map),
		(data) => ({
			type: "type_constrained",
			constraints: [
				data[0],
				data[1],
			],
		}),
	),
	type_named,
	hardval,
	constraint_map,
	// mapData( // have no idea if grammar can accept this consistently
	// 	join(LPAREN, typeval, RPAREN),
	// 	(data) => data[1],
	// ),
)));

const type_callable = rule("type_callable", mapData(
	join(typeval_atom, ARROW, typeval_atom),
	(data) => ({
		type: "type_map",
		leaf_type: undefined,
		call_input_type: data[0],
		call_output_type: data[2],
		sym_inputs: new Map(), // todo: do i want to switch from object to Map everywhere?
	}),
));

typeval.define(rule("typeval", or(
	type_callable,
	typeval_atom,
)));

const type_map_body_braced = rule("type_map_body_braced", mapData(
	join(
		LBRACE,
		RBRACE,
	),
	(_data) => ({
		type: "type_map_body",
	}),
));

const type_map_body_tupled = rule("type_map_body_tupled", mapData(
	join(
		LPAREN,
		RPAREN,
	),
	(_data) => ({
		type: "type_map_body",
	}),
));

const type_map_body = rule("type_map_body", or(
	type_map_body_braced,
	type_map_body_tupled,
));

const type_map_callable = rule("type_map_callable", mapData( // outdated not using anymore probably.
	join(
		type_map_body,
		ARROW,
		opt(type_reference),
		// note: no need for third part for mixing type reference and map type, as map type can incorporate type reference leaf in future.
		constraint_map,
	),
	(data) => ({
		type: "type_map_callable",
		call_input_type: data[0],
		call_output_type: data[2],
		call_output_constraint: data[3],
	}),
));

const top_type = rule("top_type", or(
	mapData(
		join(
			KW_TYPE,
			TYPE_IDENT,
			type_reference,
			SEMI,
		),
		(data) => ({
			type: "type",
			trail: data[1],
			definition: data[2],
		}),
	),
	mapData(
		join(
			KW_TYPE,
			TYPE_IDENT,
			type_reference,
			constraint_maybesemi,
		),
		(data) => ({
			type: "type",
			trail: data[1],
			definition: data[2],
			constraint: data[3],
		}),
	),
	mapData(
		join(
			KW_TYPE,
			TYPE_IDENT,
			constraint_maybesemi,
		),
		(data) => ({
			type: "type",
			trail: data[1],
			constraint: data[2],
		}),
	),
));

// a value is just a stricter version of a type, but it's still a type from the compiler's perspective. (value constraint perhaps i'll call it constraint instead of value).

const top_types = rule("top_types", mapData(
	join(
		KW_TYPES,
		TYPES_IDENT,
		or(
			mapData(
				join(TYPES_IDENT, SEMI),
				(data) => ({
					type: "types_reference",
					mode: "types_ident",
					trail: data[0],
				}),
			),
			mapData(
				join(KW_IMPORT, PATH, SEMI),
				(data) => ({
					type: "types_reference",
					mode: "anon_import",
					path: data[1],
					forwarding: undefined,
				}),
			),
			mapData(
				join(KW_IMPORT, PATH, forwarding),
				(data) => ({
					type: "types_reference",
					mode: "anon_import",
					path: data[1],
					forwarding: data[2],
				}),
			),
		),
	),
	(data) => ({
		type: "top_types",
		trail: data[1],
		definition: data[2],
	}),
));

const top_mod = rule("top_mod", mapData(
	join(
		KW_MOD,
		MOD_IDENT,
		or(
			mapData(
				join(KW_IMPORT, PATH, SEMI),
				(data) => ({
					type: "mod_reference",
					mode: "anon_import",
					path: data[1],
					forwarding: undefined,
				}),
			),
			mapData(
				join(KW_IMPORT, PATH, forwarding),
				(data) => ({
					type: "mod_reference",
					mode: "anon_import",
					path: data[1],
					forwarding: data[2],
				}),
			),
		),
	),
	(data) => ({
		type: "top_mod",
		trail: data[1],
		definition: data[2],
	}),
));

const case_ = rule("case", or(
	mapData(
		join(
			symbol_path,
			type_reference,
			SEMI,
		),
		(data) => ({
			type: "case",
			symbol_path: data[0],
			type_reference: data[1],
		}),
	),
	mapData(
		join(
			symbol_path,
			type_reference,
			constraint_maybesemi,
		),
		(data) => ({
			type: "case",
			symbol_path: data[0],
			type_reference: data[1],
			constraint: data[2],
		}),
	),
));

const top_extension = rule("top_extension", mapData(
	join(
		EXTAT,
		TYPE_IDENT,
		case_,
	),
	(data) => ({
		type: "top_extension",
		target_type: data[1],
		case: data[2],
	})
));

const top_create = rule("top_create", mapData(
	join(KW_CREATE, type_callable),
	(data) => ({
		type: "top_create",
		description: data[1],
	}),
));

const top_entry = rule("top_entry", or(
	top_forwarding,
	top_type,
	top_types,
	top_mod,
	top_extension,
	top_create,
	SEMI,
));

// i wonder if values and maps could be combined?
// but for now, value_map will just be optional after type.

const file_root = rule("file_root", mapData(join(opt_multi(top_entry), SKIPPERS), data => data[0]));

// SEMANTIC ANALYSIS

// COMPILER

// OUTER INTERFACE

// class ModuleLoader {
// 	constructor() {

// 	}

// 	obtain_module_files(path) {

// 	}


// }

// create_sources_scope([
// 	["//"], // reset
// 	["/", parent_scope, "/"],
// 	["/banana", parent_scope, "/prefix/on/parent/scope/"],
// 	["//octopus", parent_scope, "//prefix/on/parent/scope/"],

// ]);

const make_structured = (entries) => {
	const structured = {
		structure: entries,
		forwarding: [],
		mod_list: [],
		types_list: [],
		create: undefined,
	};

	for (const entry of entries) {
		if (entry.type === "top_forwarding") {
			structured.forwarding = [
				...structured.forwarding,
				...entry.entries,
			];
		}

		if (entry.type === "top_mod") {
			structured.mod_list.push({
				...entry,
			});
		}

		if (entry.type === "top_types") {
			structured.types_list.push({
				...entry,
			});
		}

		if (entry.type === "top_create") {
			if (structured.create !== undefined) {
				throw_error(error_internal("Only one create block permitted per module."));
			}

			structured.create = entry;
		}
	}

	return structured;
};

const augment = async (data, create_source_scope) => {
	console.log("scop", data.source_scopes, data);
	const module_source_scope = create_source_scope([
		// ["//", data.source_scopes.external, "//"],
		// Todo: i need to add the above back in. It would normally steal this from parent module, or if root then match internal scope. So we need to just pass relevant data through the system to know what mode we are in.
		["//", data.source_scopes.internal, "/"], // temp solution
		["/", data.source_scopes.internal, "/"],
	]);

	const forwarding_source_scope = create_source_scope(data.forwarding.map(entry => {
		if (entry.type === "reset") {
			return [entry.source];
		} else if (entry.type === "redirect") {
			return [entry.source, module_source_scope, entry.destination];
		}

		console.warn("Entry:", entry);
		throw_error(error_internal("Assertion error: Unhandled case."));
	}));

	const result = {
		...data,
		source_scopes: {
			...data.source_scopes ?? [],
			module: module_source_scope,
			forwarding: forwarding_source_scope,
		},
	};

	for (const mod_entry of result.mod_list) {
		if (mod_entry.definition.mode === "anon_import") {
			mod_entry.definition.uuid = await module_source_scope.resolve(mod_entry.definition.path);
		}
	}

	for (const types_entry of result.types_list) {
		if (types_entry.definition.mode === "anon_import") {
			types_entry.definition.uuid = await module_source_scope.resolve(types_entry.definition.path);
		}
	}

	return result;
};

export const process = async (config) => {
	const path = config.src;
	const actual_path = node__path.resolve(path);
	console.log("[FE] Processing module located at path:", actual_path);

	const dir_node_translator = create_dir_node_translator({
		fs,
		path: node__path,
		randomUUID,
	});

	const root_uuid = await dir_node_translator.add(actual_path);

	const create_source_scope = (redirects) => orig_create_source_scope({
		dir_node_translator,
	}, redirects);

	const file_system_source_scope = create_fs_source_scope({
		fs,
		path: node__path,
		dir_node_translator,
	}, actual_path);

	const internal_source_scope = create_source_scope([
		["//", file_system_source_scope, "./"], // todo: this will be made more exhaustive at a later stage.
		["/", file_system_source_scope, "./"],
	]);

	const source_extractor = create_source_extractor({
		fs,
		dir_node_translator,
	});

	const module_uuid = await internal_source_scope.resolve("/");
	const result = await source_extractor.extract(module_uuid);

	console.log(result);

	const parse_results = [];

	for (const file of result.files) {
		console.log("Parsing file", file);

		const text = (await fs.readFile(node__path.join(actual_path, file), "utf-8")).replaceAll("\r\n", "\n");
		const result = file_root(text);

		if (false
			|| result.success === false
			|| result.input !== ""
		) {
			console.error(result);
			console.log("<start of trace>");
			trace_print(console.log);
			console.log("<end of trace>");
			
			const json_path = node__path.join(actual_path, "frontend.trace.json");
			const text_path = node__path.join(actual_path, "frontend.trace.txt");
			
			await trace_dump(async (content) => await fs.writeFile(json_path, content, "utf-8"));
			await trace_print(async (line) => await fs.appendFile(text_path, line + "\n", "utf-8"))
			
			throw new Error(`Failed to parse.`);
		}

		parse_results.push(result.data);
	}

	console.log("Augmenting module data...");

	const combined = parse_results.flat();

	const structured = make_structured(combined);

	structured.module_path = actual_path;
	structured.module_uuid = root_uuid;

	structured.source_scopes = {
		internal: internal_source_scope,
	};

	const final = await augment(structured, create_source_scope);

	return {
		parse_output: final,
		dir_node_translations: await dir_node_translator.dump_translations(),
		module_path: actual_path,
	};
};
