import { create_unsuspended_factory } from "./uoe/create_unsuspended_factory.js";
import { throw_error } from "./uoe/throw_error.js";
import { error_user_payload } from "./uoe/error_user_payload.js";
import { error_user_state } from "./uoe/error_user_state.js";

const validate_redirect = (redirect) => {
	if (false
		|| !Array.isArray(redirect)
		|| typeof redirect[0] !== "string"
		|| (true
			&& redirect[1] !== undefined
			&& typeof redirect[1] !== "function"
			&& typeof redirect[1] !== "object"
		)
		|| (true
			&& redirect[2] !== undefined
			&& typeof redirect[2] !== "string"
		)
	) {
		return false;
	}

	return true;
};

const validate_redirects = (redirects) => {
	if (false
		|| !Array.isArray(redirects)
		|| redirects.some(redirect => !validate_redirect(redirect))
	) {
		throw_error(error_user_payload("Validation error."));
	}
};

const validate_path = (path) => {
	if (false
		|| typeof path !== "string"
		|| !/^[a-z0-9_\/.]*$/.test(path)
		|| path.includes("..") // prevent user from escaping sandbox.
	) {
		throw_error(error_user_payload("Validation error."));
	}
};

class SourceScope {
	async _init(dependencies, redirects) {
		validate_redirects(redirects);

		this._dependencies = dependencies;

		const optimized_redirects = [];

		for (const [prefix, parent_scope, replacement] of redirects) {
			if (parent_scope === undefined) {
				optimized_redirects.push([prefix, undefined]);
			} else {
				const uuid = await parent_scope.resolve(replacement ?? prefix);
				optimized_redirects.push([prefix, uuid]);
			}
		}

		// the goal is to get the longer prefixes first to ensure correct precedence.
		// todo: is this exactly how I want it?
		this._redirects = optimized_redirects.sort((a, b) => b[0].length - a[0].length);
	}

	async resolve(path) {
		validate_path(path);

		for (const [prefix, uuid] of this._redirects) {
			if (path.startsWith(prefix)) {
				if (uuid === undefined) {
					throw_error(error_user_state(`Path "${path}" has been reset, preventing you from escaping the predefined boundary. Did the creator of this scope forget to provide additional redirects (if applicable)?`));
				}

				const remainder = path.slice(prefix.length);
				return await this._dependencies.dir_node_translator.add_abstract(uuid, remainder);
			}
		}

		throw_error(error_user_state(`Path "${path}" does not play well with the predefined redirects. Did the creator of this scope forget to provide a catch-all (if applicable)?`));
	}
}

export const create_source_scope = create_unsuspended_factory(SourceScope);
