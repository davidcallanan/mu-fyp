#include "merge_smooth_void_voidptr_with_map_reference.hpp"

#include <cstdio>
#include <cstdlib>
#include "t_smooth.hpp"
#include "t_types.hpp"

std::shared_ptr<SmoothVoidptr> merge_smooth_void_voidptr_with_map_reference(
	std::shared_ptr<IrGenCtx> igc,
	std::shared_ptr<SmoothVoidVoidptr> voidptr,
	std::shared_ptr<SmoothMapReference> map_reference
) {
	return std::make_shared<SmoothVoidptr>(SmoothVoidptr{
		voidptr->type,
		map_reference->value,
	});
}
