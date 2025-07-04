#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool bool_clause(FznInvariantGraph&,
                 const std::shared_ptr<fznparser::BoolVarArray>& as,
                 const std::shared_ptr<fznparser::BoolVarArray>& bs);

bool bool_clause_reif(FznInvariantGraph&,
                 const std::shared_ptr<fznparser::BoolVarArray>& as,
                 const std::shared_ptr<fznparser::BoolVarArray>& bs,
                 const fznparser::BoolArg& reif);

bool bool_clause(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
