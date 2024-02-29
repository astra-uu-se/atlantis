#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_or(FznInvariantGraph&,
                   const fznparser::BoolVarArray& boolVarArray,
                   const fznparser::BoolArg& reified);

bool array_bool_or(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
