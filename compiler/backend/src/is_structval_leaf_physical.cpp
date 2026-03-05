#include "is_structval_leaf_physical.hpp"
#include "t_smooth.hpp"
#include "smooth_type.hpp"
#include "is_type_singletonish.hpp"

bool is_structval_leaf_physical(std::shared_ptr<SmoothStructval> structval) {
	return (true
		&& structval->has_leaf
		&& !is_type_singletonish(smooth_type(structval->leaf.value()))
	);
}
