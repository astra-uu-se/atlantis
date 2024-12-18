#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_times(FznInvariantGraph&, const fznparser::IntArg& a,
               const fznparser::IntArg& b, const fznparser::IntArg& product);

bool int_times(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
