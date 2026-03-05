#pragma once

#include <memory>
#include <variant>

struct SmoothStructval;
struct SmoothPointer;
struct SmoothEnum;
struct SmoothInt;
struct SmoothFloat;
struct SmoothVoid;

// Forward declaration impossible without pointer indirection, typical C++.

using Smooth = std::variant<
	std::shared_ptr<SmoothStructval>,
	std::shared_ptr<SmoothPointer>,
	std::shared_ptr<SmoothEnum>,
	std::shared_ptr<SmoothInt>,
	std::shared_ptr<SmoothFloat>,
	std::shared_ptr<SmoothVoid>
>;
