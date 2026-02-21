#include <cstdio>
#include <cstdlib>
#include <typeinfo>
#include "promote_to_underlying.hpp"

UnderlyingType promote_to_underlying(const Type& type) {
	if (auto p = std::get_if<std::shared_ptr<TypeMap>>(&type)) {
		return *p;
	}

	if (auto p = std::get_if<std::shared_ptr<TypePointer>>(&type)) {
		return *p;
	}

	if (auto p = std::get_if<std::shared_ptr<TypeMerged>>(&type)) {
		return *p;
	}

	const char* name = std::visit([](auto&& v) { return typeid(*v).name(); }, type);
	fprintf(stderr, "Did not encounter map or pointer during promotion to underlying, received %s\n", name);
	exit(1);
}
