#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"
#include "invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool set_in(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntSetArg&);

bool set_in(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn