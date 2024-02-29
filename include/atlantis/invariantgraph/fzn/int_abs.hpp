#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/views/intAbsNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_abs(FznInvariantGraph&, const fznparser::IntArg& var,
             const fznparser::IntArg& absVar);

bool int_abs(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
