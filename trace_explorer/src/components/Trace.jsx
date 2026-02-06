import { For, Show } from "solid-js";
import { obtain_component } from "../obtain_component.js";

export const perform_tree_building = (trace) => {
	const root = {
		entry: { type: "enter", depth: -1, position: 0, rule_name: "root" },
		children: [],
		index: -1,
	};
	
	const stack = [root];
	
	for (let i = 0; i < trace.length; i++) {
		const entry = trace[i];
		
		if (entry.type === "enter") {
			const node = {
				entry,
				children: [],
				index: i,
			};
			
			stack[stack.length - 1].children.push(node);
			stack.push(node);
		} else {
			const node = stack.pop();
			
			if (node) {
				node.exit_entry = entry;
			}
		}
	}
	
	return root;
};

export const render_path_beautifully = (path) => {
	return path.length === 0 ? "(root)" : path.map((seg) => seg.rule_name).join(" → ");
};

const stringify_path = (path) => {
	return JSON.stringify(path.map((seg) => seg.index));
};

export const find_deepest_lonely_path = (root) => {
	const path = [];
	let current = root;
	
	while (current.children.length === 1) {
		const child = current.children[0];
		
		path.push({ 
			index: 0, 
			rule_name: child.entry.rule_name ?? "UNTITLED" 
		});
		
		current = child;
	}
	
	return path;
};

export const Trace = obtain_component(({ self, props }) => {
	const { tree, expanded_paths, current_path, toggle_expand } = props;
	
	const render_node = (node, path, depth, parent_child_count) => {
		const is_lonely = parent_child_count === 1;
		const success = node.exit_entry?.success;
		const failed = success === false;
		const consumed = node.exit_entry?.consumed || 0;
		
		const render_snippet = () => {
			const snippet = node.entry.snippet;
			if (!snippet) return null;
			
			const display_snippet = snippet.substring(0, 50);
			
			if (consumed === 0) {
				return <span class="text-gray-500 text-[10px] ml-1">{display_snippet}...</span>;
			}
			
			const consumed_part = display_snippet.substring(0, consumed);
			const remaining_part = display_snippet.substring(consumed);
			
			return (
				<span class="text-[10px] ml-1">
					<span class="bg-green-200 font-semibold">{consumed_part}</span>
					<span class="text-gray-500">{remaining_part}...</span>
				</span>
			);
		};
		
		return (
			<div class="ml-2 border-l-2 border-gray-400 pl-1">
				<div
					class={`py-0.5 ${is_lonely ? "" : "cursor-pointer hover:bg-gray-200"} ${stringify_path(current_path()) === stringify_path(path) ? "bg-yellow-100" : ""}`}
					onClick={(e) => {
						if (!is_lonely) {
							e.stopPropagation();
							toggle_expand(path);
						}
					}}
				>
					<span class="inline-block w-3 text-center text-[10px]">
						{node.children.length > 0 ? ((expanded_paths().has(stringify_path(path)) || is_lonely) ? "▼" : "▶") : "•"}
					</span>
					<span class={`mono text-xs ${failed ? "text-red-600" : success === true ? "text-green-600" : ""}`}>
						{node.entry.rule_name || "unknown"}
					</span>
					<span class="text-gray-600 text-[10px] ml-1">
						@{node.entry.position}
					</span>
					<Show when={node.exit_entry}>
						<span class="text-gray-600 text-[10px] ml-1">
							+{consumed}
						</span>
					</Show>
					<Show when={node.entry.snippet}>
						{render_snippet()}
					</Show>
				</div>
				
				<Show when={(expanded_paths().has(stringify_path(path)) || is_lonely) && node.children.length > 0}>
					<For each={node.children}>
						{(child, index) => {
							const child_path = [...path, { index: index(), rule_name: child.entry.rule_name ?? "UNTITLED" }];
							return render_node(child, child_path, depth + 1, node.children.length);
						}}
					</For>
				</Show>
			</div>
		);
	};

	return () => (
		<div class="bg-white border-2 border-gray-400 p-4 shadow-md overflow-auto" style="max-height: 600px;">
			<h2 class="text-lg font-bold mb-3 text-gray-700 border-b border-gray-400 pb-1">Trace Tree:</h2>
			<Show when={tree()}>
				<For each={tree().children}>
					{(child, index) => {
						const child_path = [{ index: index(), rule_name: child.entry.rule_name ?? "UNTITLED" }];
						return render_node(child, child_path, 0, tree().children.length);
					}}
				</For>
			</Show>
		</div>
	);
});
