import { create_unsuspended_factory } from "./uoe/create_unsuspended_factory.js";

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
		throw new Error("Validation error.");
	}
};

const validate_path = (path) => {
	if (false
		|| typeof path !== "string"
		|| !/^[a-z0-9_\/.]*$/.test(path)
		|| path.includes("..") // prevent user from escaping sandbox.
	) {
		throw new Error("Validation error.");
	}
};

class SourceScope {
	_init(redirects) {
		validate_redirects(redirects);

		// the goal is to get the longer prefixes first to ensure correct precedence.
		this._redirects = redirects.sort((a, b) => b[0].length - a[0].length);
	}

	resolve(path) {
		validate_path(path);

		for (const [prefix, parent_scope, replacement] of this._redirects) {
			if (path.startsWith(prefix)) {
				if (false
					|| parent_scope === undefined
					|| replacement === undefined
				) {
					return undefined;
				}

				const remainder = path.slice(prefix.length);
				const new_path = replacement + remainder;

				return parent_scope.resolve(new_path);
			}
		}

		return undefined;
	}
}

export const create_source_scope = create_unsuspended_factory(SourceScope);
