#pragma once

#include <memory>
#include <string>
#include "t_types_fwd.hpp"
#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_smooth.hpp"

Smooth access_member(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothStructval> target_smooth,
	const std::string& sym
);
