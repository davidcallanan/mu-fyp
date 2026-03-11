#pragma once

#include <memory>
#include "t_types.hpp"
#include "t_ctx.hpp"

std::shared_ptr<TypeMap> clone_type_map_for_mutation(std::shared_ptr<TypeOrchCtx> toc, std::shared_ptr<TypeMap> map);
