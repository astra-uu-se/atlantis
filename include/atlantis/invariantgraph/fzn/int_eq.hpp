#pragma once

#include <fznparser/constraint.hpp>

#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_eq(FznInvariantGraph&, VarNode&, Int);

bool int_eq(FznInvariantGraph&, VarNode&, Int,
            const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, VarNode&, VarNode&);

bool int_eq(FznInvariantGraph&, VarNode&, VarNode&, VarNode& reified);

bool int_eq(FznInvariantGraph&, VarNode&, VarNode&,
            const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
