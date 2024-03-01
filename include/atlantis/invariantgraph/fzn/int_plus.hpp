#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_plus(FznInvariantGraph&, const fznparser::IntArg& a,
              const fznparser::IntArg& b, const fznparser::IntArg& sum);

bool int_plus(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
