#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/invariantNodes/intTimesNode.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_times(FznInvariantGraph&, const fznparser::IntArg base,
               const fznparser::IntArg power, const fznparser::IntArg product);

bool int_times(FznInvariantGraph& invariantGraph,
               const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn