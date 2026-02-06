import { obtain_component } from "../obtain_component.js";

const stringify_path = (path) => {
	return JSON.stringify(path.map((seg) => seg.index));
};

const generate_log = (node, path, depth, expanded_paths) => {
	const log = [];
	const indentation = " ".repeat(depth);
	const position = `@${node.entry.position}`;
	const rule_name = node.entry.rule_name ?? "UNTITLED";
	const snippet = node.entry.snippet ?? "";
	
	log.push(`${indentation}${position} ${rule_name} ${snippet}`);
	
	const path_str = stringify_path(path);
	const is_expanded = expanded_paths.has(path_str);
	
	if (is_expanded && node.children.length > 0) {
		for (let i = 0; i < node.children.length; i++) {
			const child = node.children[i];
			
			const child_path = [
				...path,
				{
					index: i,
					rule_name: child.entry.rule_name ?? "UNTITLED",
				},
			];
			
			const child_lines = generate_log(
				child,
				child_path,
				depth + 1,
				expanded_paths
			);
			
			log.push(...child_lines);
		}
	} else if (!is_expanded && node.children.length > 0) {
		log.push(`${indentation} (omitted)`);
	}
	
	if (node.exit_entry) {
		const is_success = node.exit_entry.success;
		const amt_consumed = node.exit_entry.consumed || 0;
		
		if (is_success === false) {
			log.push(`${indentation}${position} FAIL`);
		} else if (is_success === true) {
			log.push(`${indentation}${position} +${amt_consumed}`);
		}
	}
	
	return log;
};

const produce_log = (tree, expanded_paths) => {
	if (!tree) {
		return "";
	}
	
	const log = [];
	
	for (let i = 0; i < tree.children.length; i++) {
		const child = tree.children[i];
		
		const child_path = [
			{
				index: i,
				rule_name: child.entry.rule_name || "unknown",
			},
		];
		
		const child_lines = generate_log(
			child,
			child_path,
			0,
			expanded_paths
		);
		
		log.push(...child_lines);
	}
	
	return log.join("\n");
};

export const TraceLog = obtain_component(({ self, props }) => {
	const { tree, expanded_paths } = props;
	
	const log_text = () => produce_log(tree(), expanded_paths());
	
	const copy_outcome = self.create_signal("idle");
	
	const perform_copy = async () => {
		const text = log_text();
		try {
			await navigator.clipboard.writeText(text);
			copy_outcome.set("copied");
			setTimeout(() => copy_outcome.set("idle"), 2000);
		} catch (error) {
			copy_outcome.set("failed");
			setTimeout(() => copy_outcome.set("idle"), 2000);
		}
	};
	
	return () => (
		<div class="bg-white border-2 border-gray-400 p-4 shadow-md">
			<div class="flex items-center justify-between mb-3 border-b border-gray-400 pb-1">
				<h2 class="text-lg font-bold text-gray-700">Trace Log:</h2>
				<button
					onClick={perform_copy}
					classList={{
						"bg-blue-500 hover:bg-blue-600 border-blue-700": copy_outcome() === "idle" || copy_outcome() === "copied",
						"bg-red-500 hover:bg-red-600 border-red-700": copy_outcome() === "failed",
					}}
					class="text-white font-bold py-1 px-3 border-2 text-xs"
				>
					{copy_outcome() === "failed" ? "Failed!" : copy_outcome() === "copied" ? "Copied!" : "Copy"}
				</button>
			</div>
			<pre class="mono text-xs bg-gray-50 p-3 border border-gray-300 overflow-auto" style="max-height: 600px;">
				{log_text()}
			</pre>
		</div>
	);
});
