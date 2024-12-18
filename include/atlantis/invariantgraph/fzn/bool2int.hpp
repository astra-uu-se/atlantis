#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool int2bool(FznInvariantGraph&, const fznparser::BoolArg& boolArg,
              const fznparser::IntArg& intArg);

bool bool2int(FznInvariantGraph&, const fznparser::BoolArg& boolArg,
              const fznparser::IntArg& intArg);

bool bool2int(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
