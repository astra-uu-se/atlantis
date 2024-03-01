#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_min(FznInvariantGraph&, const fznparser::IntArg& a,
             const fznparser::IntArg& b, const fznparser::IntArg& maximum);

bool int_min(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
