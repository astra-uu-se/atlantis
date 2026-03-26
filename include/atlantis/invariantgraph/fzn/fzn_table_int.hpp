#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool fzn_table_int(FznInvariantGraph&,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   std::vector<std::vector<Int>>&& table);

bool fzn_table_int(FznInvariantGraph&,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   std::vector<std::vector<Int>>&& table,
                   const fznparser::BoolArg& reified);

bool fzn_table_int(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
