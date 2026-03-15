#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include "t_types_fwd.hpp"

std::optional<size_t> map_sym_idx(std::shared_ptr<TypeMap> map, const std::string& sym_name);
