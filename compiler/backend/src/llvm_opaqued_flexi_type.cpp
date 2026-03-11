#include "llvm_opaqued_flexi_type.hpp"
#include "llvm_flexi_type.hpp"
#include "llvm_value.hpp"
#include "get_underlying_type.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_ctx.hpp"
#include "t_bundles.hpp"
#include "llvm/IR/DerivedTypes.h"

llvm::Type* llvm_opaqued_flexi_type(const Smooth smooth, std::shared_ptr<IrGenCtx> igc) {
	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		Type underlying = get_underlying_type((*p_v_structval)->type);
		auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying);

		if (!p_v_map) {
			fprintf(stderr, "not map.\n");
			exit(1);
		}

		if (!(*p_v_map)->bundle_id.has_value()) {
			fprintf(stderr, "no bundle.\n");
			exit(1);
		}

		Bundle* bundle = igc->toc->bundle_registry->get((*p_v_map)->bundle_id.value());

		if (!bundle) {
			fprintf(stderr, "no bundle..\n");
			exit(1);
		}

		auto p_bundle_map = std::get_if<std::shared_ptr<BundleMap>>(bundle);

		if (!p_bundle_map) {
			fprintf(stderr, "no bundle...\n");
			exit(1);
		}

		llvm::StructType* opaque_flexi_struct_type = (*p_bundle_map)->opaque_flexi_struct_type;

		if (!opaque_flexi_struct_type || opaque_flexi_struct_type->isOpaque()) {
			fprintf(stderr, "struct is still opaque! use llvm_flexi_type instead or wait till body is populated!\n");
			exit(1);
		}

		return opaque_flexi_struct_type;
	}

	return llvm_flexi_type(smooth);
}
