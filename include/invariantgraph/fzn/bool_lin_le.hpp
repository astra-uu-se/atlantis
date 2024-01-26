#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>
#include <numeric>
#include <vector>

#include "invariantgraph/fzn/bool_lin_eq.hpp"
#include "invariantgraph/fzn/int_le.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_le(FznInvariantGraph&, std::vector<Int>&& coeffs,
                 const fznparser::BoolVarArray& inputs, Int bound);

bool bool_lin_le(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn