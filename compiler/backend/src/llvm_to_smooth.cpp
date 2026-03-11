#include "llvm_to_smooth.hpp"
#include "get_underlying_type.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "produce_call_func.hpp"
#include "is_type_singletonish.hpp"
#include "evaluate_singletonish.hpp"
#include "rotten_int_info.hpp"
#include "rotten_float_info.hpp"
#include "llvm/IR/DerivedTypes.h"

// this is such a hacky system and needs to be rid of at some point.

Smooth llvm_to_smooth(std::shared_ptr<IrGenCtx> igc, const Type& type, llvm::Value* value) {
	Type underlying = get_underlying_type(type);

	if (auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&underlying)) {
		const std::string& type_str = (*p_v_rotten)->type_str;

		const bool is_float = (false
			|| type_str == "f16"
			|| type_str == "f32"
			|| type_str == "f64"
			|| type_str == "f128"
		);

		if (value->getType()->isStructTy()) { // really type information should be telling us this, but this hack will do for now.
			if (auto info = rotten_float_info(*p_v_rotten)) {
				llvm::Type* flexi_type = nullptr;
				
				if (info->bits == 16) flexi_type = llvm::Type::getHalfTy(value->getContext());
				else if (info->bits == 32) flexi_type = llvm::Type::getFloatTy(value->getContext());
				else if (info->bits == 64) flexi_type = llvm::Type::getDoubleTy(value->getContext());
				else if (info->bits == 128) flexi_type = llvm::Type::getFP128Ty(value->getContext());
				
				return std::make_shared<SmoothVoidFloat>(SmoothVoidFloat{
					type,
					flexi_type,
					value,
				});
			} else if (auto info = rotten_int_info(*p_v_rotten)) {
				llvm::Type* flexi_type = llvm::IntegerType::get(value->getContext(), info->bits);
				
				return std::make_shared<SmoothVoidInt>(SmoothVoidInt{
					type,
					flexi_type,
					value,
				});
			}
			
			fprintf(stderr, "Failed to extract info from rotten.");
			exit(1);
		}

		if (is_float) {
			return std::make_shared<SmoothFloat>(SmoothFloat{
				type,
				value,
			});
		}

		return std::make_shared<SmoothInt>(SmoothInt{
			type,
			value,
		});
	}

	if (auto p_v_enum = std::get_if<std::shared_ptr<TypeEnum>>(&underlying)) {
		return std::make_shared<SmoothEnum>(SmoothEnum{
			type,
			value,
		});
	}

	if (std::get_if<std::shared_ptr<TypePointer>>(&underlying)) {
		if (value->getType()->isStructTy()) { // nice little hack here.	
			llvm::Type* flexi_type = llvm::PointerType::get(value->getContext(), 0); // 0 is address space (I think this means CPU).
		
			return std::make_shared<SmoothVoidPointer>(SmoothVoidPointer{
				type,
				flexi_type,
				value,
			});
		}

		return std::make_shared<SmoothPointer>(SmoothPointer{
			type,
			value,
		});
	}

	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&underlying)) {
		bool has_leaf = (*p_v_map)->leaf_type.has_value();
		
		if (true
			&& (*p_v_map)->leaf_hardval.has_value()
			&& !(*p_v_map)->leaf_type.has_value()
		) {
			fprintf(stderr, "serious invariant violation - new refactor does not support hardval without type.\n");
			exit(1);
		}

		std::optional<Smooth> leaf = std::nullopt;
		std::vector<Smooth> field_smooths;
		unsigned int field_index = 0;

		if (has_leaf) {
			if (is_type_singletonish((*p_v_map)->leaf_type.value())) {
				leaf = evaluate_singletonish(igc, (*p_v_map)->leaf_type.value());
			} else {
				llvm::Value* leaf_value = igc->builder->CreateExtractValue(value, field_index);
				leaf = llvm_to_smooth(igc, (*p_v_map)->leaf_type.value(), leaf_value);
				field_smooths.push_back(leaf.value());
				field_index += 1;
			}
		}

		for (const auto& [sym_name, sym_type] : (*p_v_map)->sym_inputs) {
			if (is_type_singletonish(*sym_type)) {
				continue;
			}
			
			llvm::Value* sym_value = igc->builder->CreateExtractValue(value, field_index);
			field_smooths.push_back(llvm_to_smooth(igc, *sym_type, sym_value));
			
			field_index += 1;
		}
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			value,
			has_leaf,
			leaf,
			[igc, v_map = *p_v_map]() mutable -> llvm::Function* {
				return produce_call_func(igc, v_map);
			},
			[igc, v_map = *p_v_map]() mutable -> llvm::Function* {
				return produce_call_func(igc, v_map, true);
			},
			field_smooths,
		});
	}

	const char* name = std::visit([](auto&& v) { return typeid(*v).name(); }, type);
	const char* name2 = std::visit([](auto&& v) { return typeid(*v).name(); }, underlying);
	fprintf(stderr, "not handled llvm_to_smooth.\n");
	fprintf(stderr, "got %s underlying as %s\n", name, name2);
	exit(1);
}
