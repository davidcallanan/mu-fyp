#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include "t_types_fwd.hpp"
#include "t_smooth_fwd.hpp"

std::optional<size_t> structval_field_idx(std::shared_ptr<SmoothStructval> structval, const std::string& sym_name);
