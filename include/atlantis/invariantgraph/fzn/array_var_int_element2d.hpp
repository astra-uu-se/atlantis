#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_int_element2d(
    FznInvariantGraph&, const fznparser::IntArg& rowIndex,
    const fznparser::IntArg& colIndex,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    const fznparser::IntArg& output, Int numCols, Int rowOffset, Int colOffset);

bool array_var_int_element2d(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
