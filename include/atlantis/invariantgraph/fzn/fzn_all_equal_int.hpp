#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fzn/int_eq.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolXorNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_all_equal_int(FznInvariantGraph&,
                       const fznparser::IntVarArray& inputs);

bool fzn_all_equal_int(FznInvariantGraph&, const fznparser::IntVarArray& inputs,
                       const fznparser::BoolArg& reified);

bool fzn_all_equal_int(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
