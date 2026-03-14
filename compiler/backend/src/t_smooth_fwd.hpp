#pragma once

#include <memory>
#include <variant>

struct SmoothStructval;
struct SmoothPointer;
struct SmoothEnum;
struct SmoothInt;
struct SmoothFloat;
struct SmoothVoid;
struct SmoothVoidInt;
struct SmoothVoidFloat;
struct SmoothVoidPointer;
struct SmoothMapReference;
struct SmoothVoidptr;
struct SmoothVoidVoidptr;
struct SmoothVoidMapReference;

// Forward declaration impossible without pointer indirection, typical C++.

using Smooth = std::variant<
	std::shared_ptr<SmoothStructval>,
	std::shared_ptr<SmoothPointer>,
	std::shared_ptr<SmoothEnum>,
	std::shared_ptr<SmoothInt>,
	std::shared_ptr<SmoothFloat>,
	std::shared_ptr<SmoothVoid>,
	std::shared_ptr<SmoothVoidInt>,
	std::shared_ptr<SmoothVoidFloat>,
	std::shared_ptr<SmoothVoidPointer>,
	std::shared_ptr<SmoothMapReference>,
	std::shared_ptr<SmoothVoidptr>,
	std::shared_ptr<SmoothVoidVoidptr>,
	std::shared_ptr<SmoothVoidMapReference>
>;
