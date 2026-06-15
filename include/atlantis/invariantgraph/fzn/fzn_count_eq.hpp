#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool fzn_count_eq(FznInvariantGraph&,
                  const std::shared_ptr<fznparser::IntVarArray>& inputs,
                  const fznparser::IntArg& needle,
                  const fznparser::IntArg& amount);

bool fzn_count_eq_reif(FznInvariantGraph&,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs,
                       const fznparser::IntArg& needle,
                       const fznparser::IntArg& amount,
                       const fznparser::BoolArg& reified);

bool fzn_count_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
