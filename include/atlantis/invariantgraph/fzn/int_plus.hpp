#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_plus(FznInvariantGraph&, const fznparser::IntArg& a,
              const fznparser::IntArg& b, const fznparser::IntArg& sum);

bool int_plus(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
