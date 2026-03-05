#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include "llvm/IR/DerivedTypes.h"
#include "access_member.hpp"
#include "t_types.hpp"
#include "t_smooth.hpp"
#include "get_underlying_type.hpp"
#include "llvm_to_smooth.hpp"
#include "is_type_singletonish.hpp"
#include "evaluate_singletonish.hpp"

Smooth access_member(
	IrGenCtx& igc,
	std::shared_ptr<SmoothStructval> target_smooth,
	const std::string& sym
) {
	if (auto p_v_map = std::get_if<std::shared_ptr<TypeMap>>(&target_smooth->type)) {
		const auto& v_map = *p_v_map;
		
		std::string sym_key = ":" + sym;
		
		if (v_map->sym_inputs.find(sym_key) == v_map->sym_inputs.end()) {
			fprintf(stderr, "Symbol %s not really available here", sym.c_str());
			exit(1);
		}
		
		const Type& unclear_type = *v_map->sym_inputs.at(sym_key);
		Type sym_type = get_underlying_type(unclear_type);

		if (is_type_singletonish(unclear_type)) {
			return evaluate_singletonish(igc, unclear_type);
		}

		// i know this logic is terrible but performance is not a concern for me.
		
		size_t field_index = (target_smooth->has_leaf ? 1 : 0);
		
		for (const auto& [sym_name, sym_type] : v_map->sym_inputs) {
			if (sym_name == sym_key) {
				break;
			}

			if (is_type_singletonish(*sym_type)) {
				continue;
			}

			field_index++;
		}
		
		llvm::Value* extracted = igc.builder.CreateExtractValue(target_smooth->value, field_index);

		// pointers are typically disallowed on their own, and must be wrapped into a leaf map.
		// exception is made for syms of maps to prevent infinite recursion.
		// this is why we must deal with raw pointer here and wrap it back up.
		// we gradually wrap up pointers as they are used, lazily.
		
		if (auto p_v_pointer = std::get_if<std::shared_ptr<TypePointer>>(&sym_type)) {
			fprintf(stderr, "is it even getting here. %s\n", sym.c_str());
			
			llvm::StructType* wrapped_pointer = llvm::StructType::get(igc.context, llvm::ArrayRef<llvm::Type*>{ extracted->getType() });
			llvm::Value* final_pointer = llvm::UndefValue::get(wrapped_pointer);
			final_pointer = igc.builder.CreateInsertValue(final_pointer, extracted, 0);

			auto actual_map = std::make_shared<TypeMap>(TypeMap{
				unclear_type,
				std::nullopt,
				nullptr,
				nullptr,
				{},
				{},
			});

			return std::make_shared<SmoothStructval>(SmoothStructval{
				actual_map,
				final_pointer, // todo: is this the actual struct
				true,
				std::make_shared<SmoothPointer>(SmoothPointer{
					unclear_type,
					extracted,
				}),
			});
		}

		return llvm_to_smooth(unclear_type, extracted);
	} else {
		fprintf(stderr, "Impossible to call a non-map with a symbol, what are you doing?\n");       
		exit(1);
	}
}
