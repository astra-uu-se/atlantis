#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeCircuitImplicitNode(FznInvariantGraph&,
                             const std::shared_ptr<fznparser::IntVarArray>&);

bool makeCircuitImplicitNode(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
