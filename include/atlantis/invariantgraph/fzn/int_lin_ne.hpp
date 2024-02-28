#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fzn/int_ne.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_lin_ne(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound);

bool int_lin_ne(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound,
                const fznparser::BoolArg& reified);

bool int_lin_ne(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
