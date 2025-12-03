import { create_unsuspended_factory } from "./uoe/create_unsuspended_factory.js";
import { throw_error } from "./uoe/throw_error.js";
import { error_user_state } from "./uoe/error_user_state.js";

class DirNodeTranslator {
	_init(dependencies) {
		this._dependencies = dependencies;
		this._ino_to_uuid = new Map();
		this._uuid_to_path = new Map();
		this._uuid_to_ino = new Map();
		this._path_to_uuid = new Map();
	}

	async add(path) {
		const absolute_path = this._dependencies.path.resolve(path);

		const existing_uuid_by_path = this._path_to_uuid.get(absolute_path);

		if (existing_uuid_by_path !== undefined) {
			return existing_uuid_by_path;
		}

		try {
			var stats = await this._dependencies.fs.stat(absolute_path);
		} catch (error) {
			if (error.code === "ENOENT") {
				throw_error(error_user_state(`Directory non-existent on filesystem: ${absolute_path}`));
			}
			throw error;
		}

		const existing_uuid = this._ino_to_uuid.get(stats.ino);

		if (existing_uuid !== undefined) {
			return existing_uuid;
		}

		const uuid = this._dependencies.randomUUID();

		this._ino_to_uuid.set(stats.ino, uuid);
		this._uuid_to_path.set(uuid, absolute_path);
		this._uuid_to_ino.set(uuid, stats.ino);
		this._path_to_uuid.set(absolute_path, uuid);

		return uuid;
	}

	async add_abstract(uuid, rel_path) {
		const base_path = this._uuid_to_path.get(uuid);

		if (base_path === undefined) {
			throw_error(error_user_state(`UUID ${uuid} not recognized.`));
		}

		if (rel_path.includes("..")) {
			throw_error(error_user_state(`Safety: to prevent escaping directory of abstract path, ".." is disallowed. Todo: is this exhaustive?`));
			// todo: maybe node offers flag to force safety like Python does iirc
		}

		const combined_path = this._dependencies.path.join(base_path, rel_path);

		return await this.add(combined_path);
	}

	get(uuid) {
		return this._uuid_to_path.get(uuid);
	}

	dump_translations() {
		const translations = {};

		for (const [uuid, path] of this._uuid_to_path.entries()) {
			translations[uuid] = {
				path,
			};
		}

		return translations;
	}
}

export const create_dir_node_translator = create_unsuspended_factory(DirNodeTranslator);
