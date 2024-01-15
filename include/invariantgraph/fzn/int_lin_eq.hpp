#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fzn/int_eq.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_lin_eq(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound);

bool int_lin_eq(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound,
                fznparser::BoolArg reified);

bool int_lin_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn