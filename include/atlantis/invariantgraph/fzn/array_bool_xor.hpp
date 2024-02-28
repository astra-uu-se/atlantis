#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_xor(FznInvariantGraph&,
                    const fznparser::BoolVarArray& boolVarArray,
                    const fznparser::BoolArg& reified);

bool array_bool_xor(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn