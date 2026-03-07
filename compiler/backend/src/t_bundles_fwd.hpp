#pragma once

#include <memory>
#include <variant>

struct BundleMap;

using Bundle = std::variant<
	std::shared_ptr<BundleMap>
>;
