#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_times(FznInvariantGraph&, const fznparser::IntArg& a,
               const fznparser::IntArg& b, const fznparser::IntArg& product);

bool int_times(FznInvariantGraph& invariantGraph,
               const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
