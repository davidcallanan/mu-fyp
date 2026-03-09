#include "fresh_smooth.hpp"

#include "t_smooth.hpp"
#include "t_types.hpp"
#include "llvm_to_smooth.hpp"
#include "smooth_type.hpp"

// previously we used llvm_to_smooth for everything but we now consider this legacy.
// while it uses type information and it is a great effort to try to get as much type information determined before ir generation, it has not been sufficient.
// by having the smooth information, it now becomes possible in certain cases to properly construct the new smooth.
// we fallback to the legacy llvm_to_smooth approach for other cases, but we may eventually deprecate llvm_to_smooth in favour of the modern solution.

Smooth fresh_smooth(IrGenCtx& igc, const Smooth& old_smooth, llvm::Value* fresh_value) {
	if (auto p_v_map_reference = std::get_if<std::shared_ptr<SmoothMapReference>>(&old_smooth)) {
		return std::make_shared<SmoothMapReference>(SmoothMapReference{
			(*p_v_map_reference)->type,
			fresh_value,
			(*p_v_map_reference)->structval_type,
		});
	}

	return llvm_to_smooth(igc, smooth_type(old_smooth), fresh_value);
}
