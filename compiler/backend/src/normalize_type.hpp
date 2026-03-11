#pragma once

#include "dependencies/json.hpp"
#include "t_ctx.hpp"
#include "t_types.hpp"

using json = nlohmann::json;

// This function both parses and normalizes the JSON into a nice programmatic structure.

Type normalize_type(
	std::shared_ptr<TypeOrchCtx> toc,
	const json& typeval
);
