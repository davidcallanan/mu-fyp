#include "clone_type_map_for_mutation.hpp"
#include "t_bundles.hpp"

std::shared_ptr<TypeMap> clone_type_map_for_mutation(std::shared_ptr<IrGenCtx> igc, std::shared_ptr<TypeMap> map) {
	auto clone = std::make_shared<TypeMap>(*map);
	
	clone->bundle_id = igc->toc->bundle_registry->install(
		std::make_shared<BundleMap>(BundleMap{ nullptr, nullptr, nullptr })
	);
	
	return clone;
}
