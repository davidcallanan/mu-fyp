#include <bit>
#include <cstdlib>
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "structwrap.hpp"
#include "t_smooth.hpp"
#include "t_types.hpp"

std::shared_ptr<SmoothStructval> structwrap(IrGenCtx& igc, const Smooth& smooth) {
	if (auto p_v_structval = std::get_if<std::shared_ptr<SmoothStructval>>(&smooth)) {
		return *p_v_structval;
	}

	if (auto p_v_pointer = std::get_if<std::shared_ptr<SmoothPointer>>(&smooth)) {
		const auto& v_pointer = *p_v_pointer;
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ v_pointer->value->getType() });
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		struct_value = igc.builder.CreateInsertValue(struct_value, v_pointer->value, 0);

		auto v_map = std::make_shared<TypeMap>();
		v_map->leaf_type = v_pointer->type;

		return std::make_shared<SmoothStructval>(SmoothStructval{
			Type(v_map),
			struct_value,
			true,
			std::make_shared<SmoothPointer>(SmoothPointer{
				v_pointer->type, // todo: is this the right type.
				igc.builder.CreateExtractValue(struct_value, 0),
			}),
			nullptr,
		});
	}

	if (auto p_v_int = std::get_if<std::shared_ptr<SmoothInt>>(&smooth)) {
		const auto& v_int = *p_v_int;
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ v_int->value->getType() });
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		struct_value = igc.builder.CreateInsertValue(struct_value, v_int->value, 0);

		auto v_map = std::make_shared<TypeMap>();
		v_map->leaf_type = v_int->type;

		return std::make_shared<SmoothStructval>(SmoothStructval{
			Type(v_map),
			struct_value,
			true,
			std::make_shared<SmoothInt>(SmoothInt{
				v_int->type,
				igc.builder.CreateExtractValue(struct_value, 0),
			}),
			nullptr,
		});
	}

	if (auto p_v_float = std::get_if<std::shared_ptr<SmoothFloat>>(&smooth)) {
		const auto& v_float = *p_v_float;
		
		llvm::StructType* struct_type = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ v_float->value->getType() });
		llvm::Value* struct_value = llvm::UndefValue::get(struct_type);
		struct_value = igc.builder.CreateInsertValue(struct_value, v_float->value, 0);

		auto v_map = std::make_shared<TypeMap>();
		v_map->leaf_type = v_float->type;

		return std::make_shared<SmoothStructval>(SmoothStructval{
			Type(v_map),
			struct_value,
			true,
			std::make_shared<SmoothFloat>(SmoothFloat{
				v_float->type,
				igc.builder.CreateExtractValue(struct_value, 0),
			}),
			nullptr,
		});
	}

	fprintf(stderr, "the provided Smooth is not structwrappable.\n");
	exit(1);
}
