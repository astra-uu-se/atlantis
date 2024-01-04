#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_int_element2d(FznInvariantGraph&, const fznparser::IntArg idx1,
                             const fznparser::IntArg idx2,
                             const fznparser::IntVarArray inputs,
                             const fznparser::IntArg output, Int numRows,
                             Int offset1, Int offset2);

bool array_var_int_element2d(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn