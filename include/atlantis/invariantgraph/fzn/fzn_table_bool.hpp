#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool fzn_table_bool(FznInvariantGraph&,
                    const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                    std::vector<std::vector<bool>>&& table);

bool fzn_table_bool(FznInvariantGraph&,
                    const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                    std::vector<std::vector<bool>>&& table,
                    const fznparser::BoolArg& reified);

bool fzn_table_bool(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
