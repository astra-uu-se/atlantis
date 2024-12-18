#pragma once

#include <fznparser/constraint.hpp>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_lt(FznInvariantGraph&, Int, VarNodeId);
bool int_lt(FznInvariantGraph&, VarNodeId, Int);
bool int_lt(FznInvariantGraph&, VarNodeId, VarNodeId);
bool int_lt(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_lt(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
