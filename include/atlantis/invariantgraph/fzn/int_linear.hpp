#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_linear(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
