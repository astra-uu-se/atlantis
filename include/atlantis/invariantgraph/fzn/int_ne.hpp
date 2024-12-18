#pragma once

#include <fznparser/constraint.hpp>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_ne(FznInvariantGraph&, VarNodeId, Int);

bool int_ne(FznInvariantGraph&, VarNodeId, Int,
            const fznparser::BoolArg& reified);

bool int_ne(FznInvariantGraph&, VarNodeId, VarNodeId);

bool int_ne(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_ne(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_ne(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::BoolArg& reified);

bool int_ne(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
