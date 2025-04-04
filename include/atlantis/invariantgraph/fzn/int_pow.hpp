#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int_pow(FznInvariantGraph&, const fznparser::IntArg& base,
             const fznparser::IntArg& exponent, const fznparser::IntArg& power);

bool int_pow(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
