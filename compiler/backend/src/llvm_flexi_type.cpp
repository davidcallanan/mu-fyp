#include "llvm_flexi_type.hpp"
#include "llvm_value.hpp"
#include "t_smooth.hpp"
#include "llvm/IR/DerivedTypes.h"

// this is a definitive record of what the type would be if we were genuinely storing a value.
// it bypasses the value is poison (i.e. unpopulated) and thus having a void type or similar.
// it is only used when crossing a function boundary, because the type of the function has to remain consistent irrespective of value possibilities.

llvm::Type* llvm_flexi_type(const Smooth smooth) {
	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		const auto& field_smooths = (*p_v_structval)->field_smooths;

		if (field_smooths.empty()) {
			return llvm_value(smooth)->getType();
		}

		std::vector<llvm::Type*> field_types;
		field_types.reserve(field_smooths.size());

		for (const auto& field_smooth : field_smooths) {
			field_types.push_back(llvm_flexi_type(field_smooth));
		}

		return llvm::StructType::get(field_types[0]->getContext(), field_types);
	}

	if (auto p_v_void_int = std::get_if<std::shared_ptr<SmoothVoidInt>>(&smooth)) {
		return (*p_v_void_int)->flexi_type;
	}

	if (auto p_v_void_float = std::get_if<std::shared_ptr<SmoothVoidFloat>>(&smooth)) {
		return (*p_v_void_float)->flexi_type;
	}

	if (auto p_v_void_pointer = std::get_if<std::shared_ptr<SmoothVoidPointer>>(&smooth)) {
		return (*p_v_void_pointer)->flexi_type;
	}

	return llvm_value(smooth)->getType();
}
