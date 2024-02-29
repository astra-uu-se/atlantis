#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_and(FznInvariantGraph&, std::vector<VarNodeId>&&,
                    const fznparser::BoolArg& reified);

bool array_bool_and(FznInvariantGraph&, const fznparser::BoolVarArray&,
                    const fznparser::BoolVar& reified);

bool array_bool_and(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
