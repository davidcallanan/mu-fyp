#pragma once

#include <memory>
#include "t_ctx.hpp"
#include "t_smooth.hpp"

std::shared_ptr<SmoothVoidptr> merge_smooth_void_voidptr_with_map_reference(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothVoidVoidptr> voidptr,
	std::shared_ptr<SmoothMapReference> map_reference
);
