#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fzn/int_le.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_lin_le(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound);

bool int_lin_le(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound,
                const fznparser::BoolArg& reified);

bool int_lin_le(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
