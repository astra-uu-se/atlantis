#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_mod(FznInvariantGraph&, const fznparser::IntArg& numerator,
             const fznparser::IntArg& denominator,
             const fznparser::IntArg& remainder);

bool int_mod(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
