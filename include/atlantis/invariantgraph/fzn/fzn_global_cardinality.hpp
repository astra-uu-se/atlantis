#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fzn/array_bool_and.hpp"
#include "atlantis/invariantgraph/fzn/bool_eq.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_global_cardinality(FznInvariantGraph&,
                            const fznparser::IntVarArray& inputs,
                            std::vector<Int>&& cover,
                            const fznparser::IntVarArray& counts);

bool fzn_global_cardinality(FznInvariantGraph&,
                            const fznparser::IntVarArray& inputs,
                            std::vector<Int>&& cover,
                            const fznparser::IntVarArray& counts,
                            const fznparser::BoolArg& reified);

bool fzn_global_cardinality(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
