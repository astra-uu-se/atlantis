#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_element(FznInvariantGraph&, const fznparser::IntArg& idx,
                        std::vector<bool>&& parVector,
                        const fznparser::BoolArg& output, const Int offset);

bool array_bool_element(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn