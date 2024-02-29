#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_bool_element(FznInvariantGraph&, const fznparser::IntArg& index,
                            const fznparser::BoolVarArray& inputs,
                            const fznparser::BoolArg& output, Int offset);

bool array_var_bool_element(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
