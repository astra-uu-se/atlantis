#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

static bool int_min(FznInvariantGraph& invariantGraph,
                    const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn