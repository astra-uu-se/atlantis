#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool array_bool_element2d(FznInvariantGraph&, const fznparser::IntArg& rowIndex,
                          const fznparser::IntArg& colIndex,
                          std::vector<bool>&& parVector,
                          const fznparser::BoolArg& output, Int numRows,
                          Int rowOffset, Int colOffset);

bool array_bool_element2d(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
