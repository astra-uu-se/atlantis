#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_lin_ne(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound);

bool int_lin_ne(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound, const fznparser::BoolArg& reified);

bool int_lin_ne(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
