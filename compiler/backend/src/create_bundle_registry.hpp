#pragma once

#include <cstdint>
#include <map>
#include "t_bundles_fwd.hpp"

class BundleRegistry {
private:
	uint64_t _next_id = 0;
	std::map<uint64_t, Bundle> _bundles;

public:
	uint64_t install(Bundle bundle);
	Bundle* get(uint64_t bundle_id);
};

BundleRegistry create_bundle_registry();
