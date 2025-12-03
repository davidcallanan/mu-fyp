import { create_unsuspended_factory } from "./uoe/create_unsuspended_factory.js";
import { error_user_state } from "./uoe/error_user_state.js";
import { throw_error } from "./uoe/throw_error.js";

class SourceExtractor {
	_init(dependencies) {
		this._dependencies = dependencies;
	}

	async extract(uuid) {
		// todo: add lru cache of like 1000 entries here.

		const directory = await this._dependencies.dir_node_translator.get(uuid);
		const files = await this._dependencies.fs.readdir(directory);

		if (files.some(file => file.endsWith(".ec"))) {
			return {
				type: "SOURCE MODULE",
				uuid,
				files: files.filter(file => file.endsWith(".ec")),
			};
		} else if (files.some(file => file.endsWith(".eh"))) {
			return {
				type: "INTERFACE MODULE",
				uuid,
				files: files.filter(file => file.endsWith(".eh")),
			};
		}

		throw_error(error_user_state(`Filesystem structure at ${uuid} does not align with a source or interface module.`));
	}
}

export const create_source_extractor = create_unsuspended_factory(SourceExtractor);
