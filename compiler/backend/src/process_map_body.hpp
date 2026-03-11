#pragma once

#include "t_ctx.hpp"
#include "t_types.hpp"

void process_map_body(
	std::shared_ptr<IrGenCtx> igc,
	const TypeMap& body
);
