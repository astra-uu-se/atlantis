#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_minimum(FznInvariantGraph&, const fznparser::IntArg& output,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs);

bool array_int_minimum(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
