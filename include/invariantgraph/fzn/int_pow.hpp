#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantNodes/intPowNode.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_pow(FznInvariantGraph&, const fznparser::IntArg& base,
             const fznparser::IntArg& exponent, const fznparser::IntArg& power);

bool int_pow(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn