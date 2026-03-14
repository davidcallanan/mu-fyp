#include "merge_smooth_void_map_reference_with_voidptr.hpp"

#include <cstdio>
#include <cstdlib>
#include "t_smooth.hpp"
#include "t_types.hpp"

std::shared_ptr<SmoothMapReference> merge_smooth_void_map_reference_with_voidptr(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothVoidMapReference> map_reference,
	std::shared_ptr<SmoothVoidptr> voidptr
) {
	return std::make_shared<SmoothMapReference>(SmoothMapReference{
		map_reference->type,
		voidptr->value,
		map_reference->flexi_type,
	});
}
