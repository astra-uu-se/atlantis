#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_all_different_int(FznInvariantGraph&,
                           const fznparser::IntVarArray& inputs);

bool fzn_all_different_int(FznInvariantGraph&,
                           const fznparser::IntVarArray& inputs,
                           const fznparser::BoolArg& reified);

bool fzn_all_different_int(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn