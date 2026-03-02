#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include "t_types_fwd.hpp"

struct RottenIntInfo {
	char prefix;
	uint32_t bits;
};

std::optional<RottenIntInfo> rotten_int_info(std::shared_ptr<TypeRotten> rotten);
