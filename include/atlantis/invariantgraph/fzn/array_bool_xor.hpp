#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_xor(FznInvariantGraph&,
                    const std::shared_ptr<fznparser::BoolVarArray>&);

bool array_bool_xor(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
