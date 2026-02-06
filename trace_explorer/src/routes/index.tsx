import { createFileRoute } from "@tanstack/solid-router";
import { For, Show } from "solid-js";
import { obtain_component } from "../obtain_component.js";
import { Trace, perform_tree_building, render_path_beautifully, find_deepest_lonely_path } from "../components/Trace.jsx";
import { TraceLog } from "../components/TraceLog.jsx";

const RouteComponent = obtain_component(({ self }) => {
	const tree = self.create_signal();
	const expanded_paths = self.create_signal(new Set());
	const curr_path = self.create_signal([]);
	const history = self.create_signal([]);
	const history_starred = self.create_signal([]);
	const paste_area = self.create_signal("");
	const dialog_ref = self.create_signal();
	const dropdown_button_ref = self.create_signal();

	const perform_upload_interaction = (event) => {
		const input = event.target as HTMLInputElement;
		const file = input.files?.[0];
		
		if (file) {
			const reader = new FileReader();
			
			reader.onload = (e) => {
				try {
					const content = e.target?.result as string;
					const parsed = JSON.parse(content);
					const produced_tree = perform_tree_building(parsed);
					tree.set(produced_tree);
					expanded_paths.set(new Set());
					history.set([]);
					history_starred.set([]);
					
					const initial_path = find_deepest_lonely_path(produced_tree);
					curr_path.set(initial_path);
				} catch (error) {
					alert("Could not parse trace very smoothly.");
				}
			};
			
			reader.readAsText(file);
		}
	};

	const perform_paste = () => {
		try {
			const parsed = JSON.parse(paste_area());
			const produced_tree = perform_tree_building(parsed);
			tree.set(produced_tree);
			expanded_paths.set(new Set());
			history.set([]);
			history_starred.set([]);
			paste_area.set("");
			
			const initial_path = find_deepest_lonely_path(produced_tree);
			curr_path.set(initial_path);
		} catch (error) {
			alert("Failed to smoothly parse trace.");
		}
	};

	const path_to_string = (path) => {
		return JSON.stringify(path.map((seg) => seg.index));
	};

	const update_expand_situation = (path) => {
		const new_expanded = new Set();
		
		for (let i = 0; i < path.length; i++) {
			const ancestor_path = path.slice(0, i + 1);
			const ancestor_path_str = path_to_string(ancestor_path);
			new_expanded.add(ancestor_path_str);
		}
		
		expanded_paths.set(new_expanded);
		curr_path.set([...path]);
		
		append_history(path);
	};

	const append_history = (path) => {
		const new_entry = {
			path: [...path],
			timestamp: Date.now(),
			starred: false,
		};
		
		const new_history = [new_entry, ...history()];
		
		if (new_history.length > 256) {
			new_history.splice(256);
		}
		
		history.set(new_history);
	};

	const jump_to_desired_path = (path) => {
		const new_expanded = new Set();
		
		for (let i = 0; i <= path.length; i++) {
			const sub_path = path.slice(0, i);
			new_expanded.add(path_to_string(sub_path));
		}
		
		expanded_paths.set(new_expanded);
		curr_path.set([...path]);
		
		append_history(path);
	};

	const toggle_star = (entry) => {
		if (entry.starred) {
			history_starred.set(history_starred().filter((e) => e !== entry));
			entry.starred = false;
		} else {
			entry.starred = true;
			history_starred.set([entry, ...history_starred()]);
		}
		
		history.set([...history()]);
	};

	return () => (
		<div class="min-h-screen bg-gray-100 p-4">
			<div class="max-w-7xl mx-auto">
				<div class="flex items-center justify-between mb-4 border-b-4 border-gray-600 pb-2">
					<h1 class="text-xl font-bold text-gray-800">
						Essence C - Trace Explorer
					</h1>
					<Show when={tree()}>
						<button
							onClick={() => {
								tree.set(null);
							}}
							class="bg-red-500 hover:bg-red-600 text-white font-bold py-1 px-3 border-2 border-red-700 text-sm"
						>
							Clear Trace
						</button>
					</Show>
				</div>
				
				<Show when={!tree()}>
					<div class="bg-white border-2 border-gray-400 p-6 mb-4 shadow-md">
						<h2 class="text-xl font-bold mb-3 text-gray-700">Upload or Paste Trace</h2>
						
						<div class="mb-4">
							<label class="block mb-2 font-semibold text-gray-700">Upload File:</label>
							<input
								type="file" 
								accept=".json"
								onChange={perform_upload_interaction}
								class="border-2 border-gray-400 p-2 w-full"
							/>
						</div>
						
						<div class="mb-4">
							<label class="block mb-2 font-semibold text-gray-700">Or Paste JSON:</label>
							<textarea
								value={paste_area()}
								onInput={(e) => paste_area.set(e.currentTarget.value)}
								class="border-2 border-gray-400 p-2 w-full h-32 mono text-sm"
								placeholder="Paste trace JSON here..."
							/>
							<button
								onClick={perform_paste}
								class="mt-2 bg-blue-500 hover:bg-blue-600 text-white font-bold py-2 px-4 border-2 border-blue-700"
							>
								Load Trace
							</button>
						</div>
					</div>
				</Show>
				
				<Show when={tree()}>
					<div class="bg-white border-2 border-gray-400 p-4 mb-4 shadow-md">
						<div class="flex items-center gap-2 relative">
							<div class="mono text-xs bg-gray-50 p-2 border border-gray-300 flex-1">
								{render_path_beautifully(curr_path())}
							</div>
							<button
								ref={(el) => dropdown_button_ref.set(el)}
								onClick={() => {
									const dialog = dialog_ref();
									const button = dropdown_button_ref();
									if (dialog && button) {
										if (dialog.open) {
											dialog.close();
										} else {
											const rect = button.getBoundingClientRect();
											const container_rect = button.closest(".bg-white")?.getBoundingClientRect();
											const container_width = container_rect?.width || window.innerWidth;
											dialog.style.position = "absolute";
											dialog.style.top = `${rect.bottom + 4}px`;
											dialog.style.left = `${container_rect?.left || 0}px`;
											dialog.style.width = `${container_width}px`;
											dialog.style.margin = "0";
											dialog.showModal();
										}
									}
								}}
								class="bg-blue-500 hover:bg-blue-600 text-white font-bold py-2 px-3 border-2 border-blue-700 text-sm"
							>
								▼
							</button>
						</div>
						
						<dialog
							ref={(el) => dialog_ref.set(el)}
							onClick={(e) => {
								const dialog = dialog_ref();
								if (dialog && e.target === dialog) {
									dialog.close();
								}
							}}
							class="border-2 border-gray-400 p-4 bg-white shadow-lg backdrop:bg-transparent"
							style={{
								"max-height": "400px",
							}}
						>
							<div class="overflow-auto" style="max-height: 350px;">
								<Show when={history_starred().length > 0}>
									<div class="mb-4">
										<h4 class="text-xs font-semibold mb-2 text-gray-600">Starred</h4>
										<For each={history_starred()}>
											{(entry) => (
												<div class="mb-2 p-2 bg-yellow-50 border border-yellow-400 hover:bg-yellow-100 cursor-pointer flex justify-between items-start">
													<div
														onClick={() => {
															jump_to_desired_path(entry.path);
															dialog_ref()?.close();
														}}
														class="mono text-xs break-all flex-1"
													>
														{render_path_beautifully(entry.path)}
													</div>
													<button
														onClick={() => toggle_star(entry)}
														class="ml-2 text-xs text-gray-500 hover:text-red-600 hover:font-bold transition-colors"
													>
														X
													</button>
												</div>
											)}
										</For>
									</div>
								</Show>
								
								<For each={history()}>
									{(entry) => (
										<div class="mb-2 p-1.5 bg-gray-50 border border-gray-300 hover:bg-gray-100 flex justify-between items-start">
											<div
												onClick={() => {
													jump_to_desired_path(entry.path);
													dialog_ref()?.close();
												}}
												class="mono text-xs break-all cursor-pointer flex-1"
											>
												{render_path_beautifully(entry.path)}
											</div>
											<button
												onClick={() => toggle_star(entry)}
												class="ml-2 text-xs transition-colors"
												classList={{
													"text-gray-400 hover:text-yellow-500": !entry.starred,
													"text-yellow-500 hover:text-red-600 hover:font-bold": entry.starred
												}}
											>
												{entry.starred ? "X" : "★"}
											</button>
										</div>
									)}
								</For>
							</div>
						</dialog>
					</div>
					
					<Trace
						props={{
							tree,
							expanded_paths,
							current_path: curr_path,
							toggle_expand: update_expand_situation,
						}}
					/>
					
					<div class="mt-4">
						<TraceLog
							props={{
								tree,
								expanded_paths,
							}}
						/>
					</div>
				</Show>
			</div>
		</div>
	);
});

export const Route = createFileRoute("/")({
	component: RouteComponent
});
