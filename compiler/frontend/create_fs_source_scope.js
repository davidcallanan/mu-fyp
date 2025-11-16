import { create_unsuspended_factory } from "./uoe/create_unsuspended_factory.js";

class FsSourceScope {
	_init(dependencies, base_path) {
		this._dependencies = dependencies;
		this._base_path = base_path;
	}

	async resolve(path) {
		// todo: add lru cache of like 1000 entries here.
		const directory = this._dependencies.path.join(this._base_path, path);

		try {
			var files = await this._dependencies.fs.readdir(directory);
		} catch (e) {
			console.warn(e);
			return undefined;
		}

		if (files.some(file => file.endsWith(".ec"))) {
			return {
				type: "SOURCE MODULE",
				files: files.filter(file => file.endsWith(".ec")),
			};
		} else if (files.some(file => file.endsWith(".eh"))) {
			return {
				type: "INTERFACE MODULE",
				files: files.filter(file => file.endsWith(".eh")),
			};
		}

		return undefined;
	}
}

export const create_fs_source_scope = create_unsuspended_factory(FsSourceScope);
