#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_element2d(FznInvariantGraph&, const fznparser::IntArg& idx1,
                         const fznparser::IntArg& idx2,
                         std::vector<Int>&& parVector,
                         const fznparser::IntArg& output, const Int numRows,
                         const Int offset1, const Int offset2);

bool array_int_element2d(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn