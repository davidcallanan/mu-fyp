#include "better_leaf_agnostically_translate.hpp"

#include "t_smooth.hpp"
#include "t_types.hpp"
#include "t_ctx.hpp"
#include "get_underlying_type.hpp"
#include "leaf_agnostically_translate.hpp"
#include "extract_leaf.hpp"
#include "structwrap.hpp"
#include "is_structwrappable.hpp"

Smooth better_leaf_agnostically_translate(std::shared_ptr<IrGenCtx> igc, Smooth smooth, const Type& target_type, bool use_flexi_mode) {
	Type underlying = get_underlying_type(target_type);

	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying)) {
		return leaf_agnostically_translate(igc, smooth, *p_v_map, use_flexi_mode);
	}

	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		auto v_structval = *p_v_structval;
		
		if (!v_structval->has_leaf) {
			fprintf(stderr, "tried to translate from a structval to a non-map, but had no leaf!\n");
			exit(1);
		}

		return extract_leaf(igc, smooth);
	}

	return smooth;
}
