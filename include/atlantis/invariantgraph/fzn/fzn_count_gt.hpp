#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fzn/int_lt.hpp"
#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_count_gt(FznInvariantGraph&, const fznparser::IntVarArray& inputs,
                  const fznparser::IntArg& needle,
                  const fznparser::IntArg& count);

bool fzn_count_gt(FznInvariantGraph&, const fznparser::IntVarArray& inputs,
                  const fznparser::IntArg& needle,
                  const fznparser::IntArg& count,
                  const fznparser::BoolArg& reified);

bool fzn_count_gt(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn