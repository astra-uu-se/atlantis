#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool fzn_all_different_int(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>& inputs);

bool fzn_all_different_int(
    FznInvariantGraph&, const std::shared_ptr<fznparser::IntVarArray>& inputs,
    const fznparser::BoolArg& reified);

bool fzn_all_different_int(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
