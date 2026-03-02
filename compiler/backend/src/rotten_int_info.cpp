#include "rotten_int_info.hpp"
#include "t_types.hpp"

std::optional<RottenIntInfo> rotten_int_info(std::shared_ptr<TypeRotten> rotten) {
	if ((rotten->type_str).empty()) {
		return std::nullopt;
	}

	const char prefix = (rotten->type_str)[0];
	
	if (prefix != 'i' && prefix != 'u') {
		return std::nullopt;
	}

	return RottenIntInfo{
		prefix,
		(uint32_t) std::stoul((rotten->type_str).substr(1)),
	};
}
