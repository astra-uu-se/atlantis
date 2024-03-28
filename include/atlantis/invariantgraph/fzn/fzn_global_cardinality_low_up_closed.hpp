#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_global_cardinality_low_up_closed(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up);

bool fzn_global_cardinality_low_up_closed(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
    const fznparser::BoolArg& reified);

bool fzn_global_cardinality_low_up_closed(FznInvariantGraph&,
                                          const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
