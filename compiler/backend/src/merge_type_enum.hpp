#pragma once

#include <memory>
#include "t_types_fwd.hpp"

std::shared_ptr<TypeEnum> merge_type_enum(std::shared_ptr<TypeEnum> enum_a, std::shared_ptr<TypeEnum> enum_b);
