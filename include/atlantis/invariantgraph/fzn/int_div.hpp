#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_div(FznInvariantGraph&, const fznparser::IntArg& numerator,
             const fznparser::IntArg& denominator,
             const fznparser::IntArg& quotient);

bool int_div(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
