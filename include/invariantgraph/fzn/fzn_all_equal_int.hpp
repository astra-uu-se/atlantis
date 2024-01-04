#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fzn/int_eq.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"
#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolXorNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_all_equal_int(FznInvariantGraph&,
                       const fznparser::IntVarArray& inputs);

bool fzn_all_equal_int(FznInvariantGraph&, const fznparser::IntVarArray& inputs,
                       const fznparser::BoolArg reified);

bool fzn_all_equal_int(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn