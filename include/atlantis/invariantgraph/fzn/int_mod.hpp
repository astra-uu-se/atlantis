#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_mod(FznInvariantGraph&, const fznparser::IntArg& a,
             const fznparser::IntArg& b, const fznparser::IntArg& output);

bool int_mod(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
