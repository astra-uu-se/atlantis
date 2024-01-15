#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fzn/int_eq.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_eq(FznInvariantGraph&, std::vector<Int>&& coeffs,
                 std::vector<VarNodeId>&& inputVarNodeIds, Int sum);

bool bool_lin_eq(FznInvariantGraph&, std::vector<Int>&& coeffs,
                 std::vector<VarNodeId>&& inputVarNodeIds,
                 VarNodeId outputVarNodeId);

bool bool_lin_eq(FznInvariantGraph&, std::vector<Int>&& coeffs,
                 const fznparser::BoolVarArray inputs,
                 const fznparser::IntArg& outputVar);

bool bool_lin_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn