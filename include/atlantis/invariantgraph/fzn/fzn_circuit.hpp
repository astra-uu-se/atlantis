#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_circuit(FznInvariantGraph&,
                 const std::shared_ptr<fznparser::IntVarArray>& inputs);

bool fzn_circuit(FznInvariantGraph&,
                 const std::shared_ptr<fznparser::IntVarArray>& inputs,
                 Int offset);

bool fzn_circuit(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
