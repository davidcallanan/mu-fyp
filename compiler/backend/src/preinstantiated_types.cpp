#include "preinstantiated_types.hpp"

const std::shared_ptr<TypeEnum> type_bool = [] {
	auto v_enum = std::make_shared<TypeEnum>();
	v_enum->syms = {"false", "true"};
	v_enum->is_instantiated = true;
	return v_enum;
}();

const std::shared_ptr<TypeEnum> type_false = [] {
	auto v_enum = std::make_shared<TypeEnum>();
	v_enum->syms = {"false", "true"};
	v_enum->hardsym = "false";
	return v_enum;
}();

const std::shared_ptr<TypeEnum> type_true = [] {
	auto v_enum = std::make_shared<TypeEnum>();
	v_enum->syms = {"false", "true"};
	v_enum->hardsym = "true";
	return v_enum;
}();

const std::shared_ptr<TypeVoid> type_void = std::make_shared<TypeVoid>();

const std::shared_ptr<TypeNullptr> type_nullptr = std::make_shared<TypeNullptr>();

const std::shared_ptr<TypeVoidptr> type_voidptr = std::make_shared<TypeVoidptr>();