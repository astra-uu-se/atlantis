#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_max(FznInvariantGraph&, const fznparser::IntArg& a,
             const fznparser::IntArg& b, const fznparser::IntArg& maximum);

bool int_max(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
