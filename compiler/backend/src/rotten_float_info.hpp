#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include "t_types_fwd.hpp"

struct RottenFloatInfo {
	uint32_t bits;
};

std::optional<RottenFloatInfo> rotten_float_info(std::shared_ptr<TypeRotten> rotten);
