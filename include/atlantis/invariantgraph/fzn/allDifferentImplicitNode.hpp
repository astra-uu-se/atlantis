#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>
#include <unordered_set>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeAllDifferentImplicitNode(FznInvariantGraph&,
                                  const fznparser::IntVarArray&);

bool makeAllDifferentImplicitNode(FznInvariantGraph&,
                                  const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn