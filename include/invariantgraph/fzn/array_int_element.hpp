#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_element(FznInvariantGraph&, const fznparser::IntArg& idx,
                       std::vector<Int>&& parArray,
                       const fznparser::IntArg& output, Int offset);

bool array_int_element(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn