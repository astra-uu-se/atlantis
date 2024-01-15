#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeCircuitImplicitNode(FznInvariantGraph&, const fznparser::IntVarArray&);

bool makeCircuitImplicitNode(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn