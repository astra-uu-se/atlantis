#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_clause(FznInvariantGraph&,
                 const std::shared_ptr<fznparser::BoolVarArray>& as,
                 const std::shared_ptr<fznparser::BoolVarArray>& bs);

bool bool_clause(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
