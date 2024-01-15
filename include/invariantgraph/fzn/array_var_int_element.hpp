#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_int_element(FznInvariantGraph&, const fznparser::IntArg index,
                           const fznparser::IntVarArray& inputs,
                           const fznparser::IntArg& output, Int offset);

bool array_var_int_element(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn