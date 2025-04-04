#pragma once

#include <fznparser/constraint.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool set_in(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntSet&);

bool set_in(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntSet&, const fznparser::BoolArg& reified);

bool set_in(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
