#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_and(FznInvariantGraph&, std::vector<VarNodeId>&&,
                    const fznparser::BoolArg& reified);

bool array_bool_and(FznInvariantGraph&,
                    const std::shared_ptr<fznparser::BoolVarArray>&,
                    const fznparser::BoolVar& reified);

bool array_bool_and(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
