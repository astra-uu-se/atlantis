#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_eq(FznInvariantGraph&, VarNodeId a, VarNodeId b);

bool bool_eq(FznInvariantGraph&, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b);

bool bool_eq(FznInvariantGraph&, VarNodeId a, VarNodeId b, VarNodeId reified);

bool bool_eq(FznInvariantGraph&, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified);

bool bool_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
