#pragma once

#include <memory>
#include "t_types_fwd.hpp"

std::shared_ptr<TypeMap> merge_type_map(std::shared_ptr<TypeMap> map_a, std::shared_ptr<TypeMap> map_b);
