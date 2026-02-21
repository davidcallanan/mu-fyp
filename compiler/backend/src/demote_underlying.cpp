#include "demote_underlying.hpp"

Type demote_underlying(const UnderlyingType& type) {
	return std::visit([](auto&& v) -> Type { return v; }, type);
}
