#include "create_bundle_registry.hpp"

uint64_t BundleRegistry::install(Bundle bundle) {
	uint64_t id = _next_id++;
	_bundles[id] = bundle;
	return id;
}

Bundle* BundleRegistry::get(uint64_t bundle_id) {
	auto it = _bundles.find(bundle_id);
	
	if (it == _bundles.end()) {
		return nullptr;
	}
	
	return &it->second;
}

BundleRegistry create_bundle_registry() {
	return BundleRegistry{};
}
