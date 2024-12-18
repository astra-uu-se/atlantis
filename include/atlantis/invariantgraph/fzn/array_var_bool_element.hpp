#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class FznInvariantGraph;
}

namespace atlantis::invariantgraph::fzn {

bool array_var_bool_element(
    FznInvariantGraph&, const fznparser::IntArg& index,
    const std::shared_ptr<fznparser::BoolVarArray>& inputs,
    const fznparser::BoolArg& output, Int offset);

bool array_var_bool_element(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
