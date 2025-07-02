#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool fzn_count_leq(FznInvariantGraph&,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count);

bool fzn_count_leq_reif(FznInvariantGraph&,
                        const std::shared_ptr<fznparser::IntVarArray>& inputs,
                        const fznparser::IntArg& needle,
                        const fznparser::IntArg& count,
                        const fznparser::BoolArg& reified);

bool fzn_count_leq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
