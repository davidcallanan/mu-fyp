#include "llvm_to_smooth.hpp"
#include "get_underlying_type.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"
#include "produce_call_func.hpp"
#include "is_type_singletonish.hpp"
#include "evaluate_singletonish.hpp"

// this is such a hacky system and needs to be rid of at some point.

Smooth llvm_to_smooth(IrGenCtx& igc, const Type& type, llvm::Value* value) {
	Type underlying = get_underlying_type(type);

	if (auto p_v_rotten = std::get_if<std::shared_ptr<TypeRotten>>(&underlying)) {
		const std::string& type_str = (*p_v_rotten)->type_str;

		const bool is_float = (false
			|| type_str == "f16"
			|| type_str == "f32"
			|| type_str == "f64"
			|| type_str == "f128"
		);

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

		if (has_leaf) {
			if (is_type_singletonish((*p_v_map)->leaf_type.value())) {
				leaf = evaluate_singletonish(igc, (*p_v_map)->leaf_type.value());
			} else {
				llvm::Value* leaf_value = igc.builder.CreateExtractValue(value, 0);
				leaf = llvm_to_smooth(igc, (*p_v_map)->leaf_type.value(), leaf_value);
			}
		}
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			value,
			has_leaf,
			leaf,
			produce_call_func(igc, *p_v_map),
			{}, // todo: this needs to be fixed.
		});
	}

	const char* name = std::visit([](auto&& v) { return typeid(*v).name(); }, type);
	const char* name2 = std::visit([](auto&& v) { return typeid(*v).name(); }, underlying);
	fprintf(stderr, "not handled llvm_to_smooth.\n");
	fprintf(stderr, "got %s underlying as %s\n", name, name2);
	exit(1);
}
