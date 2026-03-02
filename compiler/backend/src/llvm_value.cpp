#include "llvm_value.hpp"
#include "t_smooth.hpp"

llvm::Value* llvm_value(const Smooth& smooth) {
	if (auto p = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		return (*p)->value;
	}
	
	if (auto p = std::get_if<std::shared_ptr<SmoothPointer>>(&smooth)) {
		return (*p)->value;
	}
	
	if (auto p = std::get_if<std::shared_ptr<SmoothEnum>>(&smooth)) {
		return (*p)->value;
	}
	
	if (auto p = std::get_if<std::shared_ptr<SmoothInt>>(&smooth)) {
		return (*p)->value;
	}
	
	if (auto p = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth)) {
		return (*p)->value;
	}
	
	fprintf(stderr, "This kind of a smooth does not have llvm value associated with it.");
	exit(1);
}
