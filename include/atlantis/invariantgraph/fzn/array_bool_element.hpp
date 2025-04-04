#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool array_bool_element(FznInvariantGraph&, const fznparser::IntArg& idx,
                        std::vector<bool>&& parVector,
                        const fznparser::BoolArg& output, Int offset);

bool array_bool_element(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
