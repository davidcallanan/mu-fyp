#include "rotten_float_info.hpp"
#include "t_types.hpp"

std::optional<RottenFloatInfo> rotten_float_info(std::shared_ptr<TypeRotten> rotten) {
	if (rotten->type_str.empty() || rotten->type_str[0] != 'f') {
		return std::nullopt;
	}

	return RottenFloatInfo{
		(uint32_t) std::stoul(rotten->type_str.substr(1)),
	};
}
