#include <cstdio>
#include <cstdlib>
#include <variant>
#include <string>
#include "evaluate_hardval.hpp"
#include "t_hardval.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"

llvm::Value* evaluate_hardval(
	IrGenCtx& igc,
	const Hardval& hardval,
	const std::string& type_str
) {
	if (std::holds_alternative<std::shared_ptr<HardvalInteger>>(hardval)) {
		const auto& p_v_int = std::get<std::shared_ptr<HardvalInteger>>(hardval);
		const std::string& value_str = p_v_int->value;
		
		int bits_needed;
		
		if (!type_str.empty()) {
			if (type_str[0] != 'i' && type_str[0] != 'u') {
				fprintf(stderr, "This received .type is not currently implemented, got %s\n", type_str.c_str());
				exit(1);
			}
			
			bits_needed = std::stoi(type_str.substr(1));
		} else {
			bool is_negative = (!value_str.empty() && value_str[0] == '-');
			std::string digits = is_negative ? value_str.substr(1) : value_str;
			
			if (is_negative) {
				llvm::APInt ap_value(128, digits.c_str(), 10);
				bits_needed = ap_value.getActiveBits() + 1;
			} else {
				llvm::APInt ap_value(128, digits.c_str(), 10);
				bits_needed = ap_value.getActiveBits();
				
				if (bits_needed == 0) {
					bits_needed = 1;
				}
			}
		}
		
		llvm::APInt ap_int(bits_needed, value_str.c_str(), 10);
		llvm::Value* const_value = llvm::ConstantInt::get(igc.context, ap_int);
		
		return const_value;
	}
	
	if (std::holds_alternative<std::shared_ptr<HardvalFloat>>(hardval)) {
		const auto& p_v_float = std::get<std::shared_ptr<HardvalFloat>>(hardval);
		
		llvm::Type* float_type;
		
		if (!type_str.empty()) {
			if (type_str[0] != 'f') {
				fprintf(stderr, "This received .type is not currently implemented, got %s\n", type_str.c_str());
				exit(1);
			}
			
			int bit_width = std::stoi(type_str.substr(1));
			
			if (bit_width == 16) {
				float_type = llvm::Type::getHalfTy(igc.context);
			} else if (bit_width == 32) {
				float_type = llvm::Type::getFloatTy(igc.context);
			} else if (bit_width == 64) {
				float_type = llvm::Type::getDoubleTy(igc.context);
			} else if (bit_width == 128) {
				float_type = llvm::Type::getFP128Ty(igc.context);
			} else {
				fprintf(stderr, "The float size is not supported, got %d\n", bit_width);
				exit(1);
			}
		} else {
			float_type = llvm::Type::getDoubleTy(igc.context);
		}
		
		llvm::APFloat ap_float(float_type->getFltSemantics(), p_v_float->value);
		llvm::Value* const_value = llvm::ConstantFP::get(igc.context, ap_float);
		
		return const_value;
	}
	
	if (std::holds_alternative<std::shared_ptr<HardvalString>>(hardval)) {
		const auto& p_v_string = std::get<std::shared_ptr<HardvalString>>(hardval);
		llvm::Value* str_const = igc.builder.CreateGlobalStringPtr(p_v_string->value);
		return str_const;
	}
	
	fprintf(stderr, "Unhandled evaluation logic for hardval\n");
	exit(1);
}
