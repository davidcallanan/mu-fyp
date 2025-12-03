import { create_unsuspended_factory } from "./uoe/create_unsuspended_factory.js";

class FsSourceScope {
	_init(dependencies, base_path) {
		this._dependencies = dependencies;
		this._base_path = base_path;
	}

	async resolve(path) {
		const directory = this._dependencies.path.join(this._base_path, path);
		return await this._dependencies.dir_node_translator.add(directory);
	}
}

export const create_fs_source_scope = create_unsuspended_factory(FsSourceScope);
