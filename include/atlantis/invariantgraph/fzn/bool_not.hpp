#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool bool_not(FznInvariantGraph&, const fznparser::BoolArg& boolVar,
              const fznparser::BoolArg& negatedBoolVar);

bool bool_not(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
