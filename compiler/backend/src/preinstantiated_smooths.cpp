#include "preinstantiated_smooths.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_bundles.hpp"
#include "t_ctx.hpp"
#include "evaluate_smooth.hpp"

std::shared_ptr<SmoothVoid> smooth_void(std::shared_ptr<IrGenCtx> igc, std::optional<Type> type) {
	llvm::StructType* struct_type = llvm::StructType::get(*igc->context, {});
	llvm::Value* value = llvm::UndefValue::get(struct_type);
	
	return std::make_shared<SmoothVoid>(SmoothVoid{
		type,
		value,
	});
}

std::shared_ptr<SmoothStructval> smooth_map_empty(std::shared_ptr<IrGenCtx> igc) {
	auto bundle_map = std::make_shared<BundleMap>();
	
	uint64_t bundle_id = igc->toc->bundle_registry->install(Bundle(bundle_map));

	auto type_map = std::make_shared<TypeMap>();
	
	type_map->bundle_id = bundle_id;

	Smooth result = evaluate_smooth(igc, Type(type_map));
	
	return std::get<std::shared_ptr<SmoothStructval>>(result);
}
