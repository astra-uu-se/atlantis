#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool fzn_global_cardinality_low_up(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up);

bool fzn_global_cardinality_low_up(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
    const fznparser::BoolArg& reified);

bool fzn_global_cardinality_low_up(FznInvariantGraph&,
                                   const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
