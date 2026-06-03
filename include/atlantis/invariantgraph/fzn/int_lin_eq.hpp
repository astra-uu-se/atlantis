#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_lin_eq(FznInvariantGraph&, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound);

bool int_lin_eq_reif(FznInvariantGraph&, std::vector<Int>&& coeffs,
                     const std::shared_ptr<fznparser::IntVarArray>& inputs,
                     Int bound, const fznparser::BoolArg& reified);

bool int_lin_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
