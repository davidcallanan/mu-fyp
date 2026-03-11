#pragma once

#include "t_hardval_fwd.hpp"
#include "t_types_fwd.hpp"
#include "t_smooth_fwd.hpp"
#include "t_ctx.hpp"

Smooth evaluate_hardval(
	std::shared_ptr<IrGenCtx> igc,
	const Hardval& hardval,
	Type type
);
