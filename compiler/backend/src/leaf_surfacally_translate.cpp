#include "leaf_surfacally_translate.hpp"

#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_ctx.hpp"
#include "get_underlying_type.hpp"
#include "is_type_singletonish.hpp"
#include "extract_leaf.hpp"
#include "structwrap.hpp"
#include "is_structwrappable.hpp"

Smooth leaf_surfacally_translate(std::shared_ptr<IrGenCtx> igc, Smooth smooth, const Type type) {
	Type underlying = get_underlying_type(type);
	auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying);
	auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth);

	if (true
		// target is WRAP
		&& p_v_map != nullptr
		&& (*p_v_map)->leaf_type.has_value()
		
		// source is a LEAF
		&& p_v_structval == nullptr
	) {
		if (!is_structwrappable(smooth)) {
			const char* sourcetypeid = std::visit([](auto&& v) { return typeid(*v).name(); }, smooth);
			const char* targettypeid = std::visit([](auto&& v) { return typeid(*v).name(); }, type);
			
			fprintf(stderr, "Failed to translate from LEAF to WRAP when LEAF not structwrappable.\n");
			fprintf(stderr, "Source (LEAF): %s\n", sourcetypeid);
			fprintf(stderr, "Target (WRAP): %s\n", targettypeid);
			exit(1);
		}
		
		smooth = structwrap(igc, smooth);
	}

	if (true
		// source is WRAP
		&& p_v_structval != nullptr
		&& (*p_v_structval)->has_leaf

		// target is a LEAF
		&& p_v_map == nullptr
	) {
		if (!(*p_v_structval)->has_leaf) {
			const char* sourcetypeid = std::visit([](auto&& v) { return typeid(*v).name(); }, smooth);
			const char* targettypeid = std::visit([](auto&& v) { return typeid(*v).name(); }, type);
			
			fprintf(stderr, "Failed to translate from WRAP to LEAF when WRAP doesn't have leaf.\n");
			fprintf(stderr, "Source (WRAP): %s\n", sourcetypeid);
			fprintf(stderr, "Target (LEAF): %s\n", targettypeid);
			exit(1);
		}
		
		smooth = extract_leaf(igc, smooth);
	}

	return smooth;
}
