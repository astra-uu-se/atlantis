#pragma once

#include <fznparser/constraint.hpp>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {
bool int_le(FznInvariantGraph&, Int, VarNodeId);
bool int_le(FznInvariantGraph&, VarNodeId, Int);
bool int_le(FznInvariantGraph&, VarNodeId, Int, const fznparser::BoolArg& reif);
bool int_le(FznInvariantGraph&, VarNodeId, VarNodeId);
bool int_le(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_le(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);
bool int_le(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
