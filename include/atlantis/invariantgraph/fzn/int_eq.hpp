#pragma once

#include <fznparser/constraint.hpp>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_eq(FznInvariantGraph&, VarNodeId, Int);

bool int_eq(FznInvariantGraph&, VarNodeId, Int,
            const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId, VarNodeId reified);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
