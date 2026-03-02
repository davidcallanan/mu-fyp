#include "is_structwrappable.hpp"
#include "t_smooth_fwd.hpp"

bool is_structwrappable(const Smooth& smooth) {
	return (false
		|| std::holds_alternative<std::shared_ptr<SmoothStructval>>(smooth)
		|| std::holds_alternative<std::shared_ptr<SmoothPointer>>(smooth)
		|| std::holds_alternative<std::shared_ptr<SmoothInt>>(smooth)
		|| std::holds_alternative<std::shared_ptr<SmoothFloat>>(smooth)
	);
}
