#pragma once

#include <memory>
#include <optional>
#include "t_ctx.hpp"
#include "t_smooth_fwd.hpp"
#include "t_types_fwd.hpp"

std::shared_ptr<SmoothVoid> smooth_void(IrGenCtx& igc, std::optional<Type> type = std::nullopt);
