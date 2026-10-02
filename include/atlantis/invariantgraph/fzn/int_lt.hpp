#pragma once

#include <fznparser/constraint.hpp>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_lt(FznInvariantGraph&, Int, VarNode&);
bool int_lt(FznInvariantGraph&, VarNode&, Int);
bool int_lt(FznInvariantGraph&, VarNode&, VarNode&);
bool int_lt(FznInvariantGraph&, VarNode&, VarNode&,
            const fznparser::BoolArg& reified);

bool int_lt(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
