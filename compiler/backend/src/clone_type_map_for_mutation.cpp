#include "clone_type_map_for_mutation.hpp"
#include "t_bundles.hpp"

std::shared_ptr<TypeMap> clone_type_map_for_mutation(std::shared_ptr<IrGenCtx> igc, std::shared_ptr<TypeMap> map) {
	auto clone = std::make_shared<TypeMap>(*map);
	
	// we can't create a new bundle - we need to share the opaque struct type!
	// hopefully nothing else in the bundle is affected by this!
	// refactor plan if this goes badly: have separate GenerationBundle and TypingBundle; move opaque struct to Typing.
	
	// wait, we have to create a new bundle, this is for cloning types for mutation.
	// it's only if we are dummy evaluating an existing type do we have to hold the old bundle.
	
	clone->bundle_id = igc->toc->bundle_registry->install(
		std::make_shared<BundleMap>(BundleMap{ nullptr, nullptr, nullptr })
	);
	
	return clone;
}
