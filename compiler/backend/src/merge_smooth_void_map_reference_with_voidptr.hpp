#pragma once

#include <memory>
#include "t_ctx.hpp"
#include "t_smooth.hpp"

std::shared_ptr<SmoothMapReference> merge_smooth_void_map_reference_with_voidptr(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothVoidMapReference> map_reference,
	std::shared_ptr<SmoothVoidptr> voidptr
);
