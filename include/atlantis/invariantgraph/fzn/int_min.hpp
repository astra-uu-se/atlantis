#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_min(FznInvariantGraph&, const fznparser::IntArg& a,
             const fznparser::IntArg& b, const fznparser::IntArg& minimum);

bool int_min(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
