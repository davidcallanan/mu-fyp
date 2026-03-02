#include "llvm_to_smooth.hpp"
#include "get_underlying_type.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"

Smooth llvm_to_smooth(const Type& type, llvm::Value* value) {
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
		bool has_leaf = (false
			|| (*p_v_map)->leaf_type.has_value()
			|| (*p_v_map)->leaf_hardval.has_value()
		);
		
		return std::make_shared<SmoothStructval>(SmoothStructval{
			type,
			value,
			has_leaf,
		});
	}

	const char* name = std::visit([](auto&& v) { return typeid(*v).name(); }, type);
	const char* name2 = std::visit([](auto&& v) { return typeid(*v).name(); }, underlying);
	fprintf(stderr, "not handled llvm_to_smooth.\n");
	fprintf(stderr, "got %s underlying as %s\n", name, name2);
	exit(1);
}
