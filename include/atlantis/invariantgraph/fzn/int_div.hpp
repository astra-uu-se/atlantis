#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_div(FznInvariantGraph&, const fznparser::IntArg& numerator,
             const fznparser::IntArg& denominator,
             const fznparser::IntArg& quotient);

bool int_div(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
