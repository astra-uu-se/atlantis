#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool array_bool_or(FznInvariantGraph&,
                   const std::shared_ptr<fznparser::BoolVarArray>& boolVarArray,
                   const fznparser::BoolArg& reified);

bool array_bool_or(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
