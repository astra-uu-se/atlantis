#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool bool_lin_le(FznInvariantGraph&, std::vector<Int>&& coeffs,
                 const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                 Int bound);

bool bool_lin_le(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
