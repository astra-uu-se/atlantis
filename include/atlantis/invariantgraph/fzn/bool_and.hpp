#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool bool_and(FznInvariantGraph&, const fznparser::BoolArg& a,
              const fznparser::BoolArg& b, const fznparser::BoolArg& reified);

bool bool_and(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
