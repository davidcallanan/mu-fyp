#include "llvm_value.hpp"
#include "t_smooth.hpp"

llvm::Value* llvm_value(const Smooth& smooth) {
	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		return (*p_v_structval)->value;
	}
	if (auto p_v_pointer = std::get_if<std::shared_ptr<SmoothPointer>>(&smooth)) {
		return (*p_v_pointer)->value;
	}
	
	if (auto p_v_enum = std::get_if<std::shared_ptr<SmoothEnum>>(&smooth)) {
		return (*p_v_enum)->value;
	}
	
	if (auto p_v_int = std::get_if<std::shared_ptr<SmoothInt>>(&smooth)) {
		return (*p_v_int)->value;
	}
	
	if (auto p_v_float = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth)) {
		return (*p_v_float)->value;
	}

	if (auto p_v_void = std::get_if<std::shared_ptr<SmoothVoid>>(&smooth)) {
		return (*p_v_void)->value;
	}

	if (auto p_v_void_int = std::get_if<std::shared_ptr<SmoothVoidInt>>(&smooth)) {
		return (*p_v_void_int)->value;
	}

	if (auto p_v_void_float = std::get_if<std::shared_ptr<SmoothVoidFloat>>(&smooth)) {
		return (*p_v_void_float)->value;
	}

	if (auto p_v_void_pointer = std::get_if<std::shared_ptr<SmoothVoidPointer>>(&smooth)) {
		return (*p_v_void_pointer)->value;
	}
	
	if (auto p_v_map_reference = std::get_if<std::shared_ptr<SmoothMapReference>>(&smooth)) {
		return (*p_v_map_reference)->value;
	}

	if (auto p_v_voidptr = std::get_if<std::shared_ptr<SmoothVoidptr>>(&smooth)) {
		return (*p_v_voidptr)->value;
	}

	if (auto p_v_void_voidptr = std::get_if<std::shared_ptr<SmoothVoidVoidptr>>(&smooth)) {
		return (*p_v_void_voidptr)->value;
	}

	fprintf(stderr, "This kind of a smooth does not have llvm value associated with it.");
	exit(1);
}
