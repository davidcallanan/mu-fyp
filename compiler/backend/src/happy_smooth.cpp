#include "happy_smooth.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "get_underlying_type.hpp"
#include "rotten_int_info.hpp"
#include "rotten_float_info.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

Smooth happy_smooth(IrGenCtx& igc, Smooth smooth, const Type& type) {
	Type underlying = get_underlying_type(type);
	auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&underlying);

	if (!p_v_rotten) {
		return smooth;
	}

	if (auto p_v_int = std::get_if<std::shared_ptr<SmoothInt>>(&smooth)) {
		if (!(*p_v_int)->value) {
			return smooth;
		}

		if (auto info = rotten_int_info(*p_v_rotten)) {
			uint32_t actual_bits = (*p_v_int)->value->getType()->getIntegerBitWidth();

			if (actual_bits < info->bits) {
				llvm::Value* extended = igc.builder.CreateZExt(
					(*p_v_int)->value,
					llvm::IntegerType::get(igc.context, info->bits)
				);
				
				return std::make_shared<SmoothInt>(SmoothInt{ (*p_v_int)->type, extended });
			}
		}

		return smooth;
	}

	if (auto p_v_float = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth)) {
		if (!(*p_v_float)->value) {
			return smooth;
		}

		if (auto info = rotten_float_info(*p_v_rotten)) {
			llvm::Type* target_type = nullptr;

			if (info->bits == 16) target_type = llvm::Type::getHalfTy(igc.context);
			else if (info->bits == 32) target_type = llvm::Type::getFloatTy(igc.context);
			else if (info->bits == 64) target_type = llvm::Type::getDoubleTy(igc.context);
			else if (info->bits == 128) target_type = llvm::Type::getFP128Ty(igc.context);

			if (target_type && (*p_v_float)->value->getType() != target_type) {
				llvm::Value* extended = igc.builder.CreateFPExt(
					(*p_v_float)->value,
					target_type
				);
				
				return std::make_shared<SmoothFloat>(SmoothFloat{ (*p_v_float)->type, extended });
			}
		}

		return smooth;
	}

	return smooth;
}
