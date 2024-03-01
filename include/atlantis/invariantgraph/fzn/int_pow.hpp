#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_pow(FznInvariantGraph&, const fznparser::IntArg& base,
             const fznparser::IntArg& exponent, const fznparser::IntArg& power);

bool int_pow(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
