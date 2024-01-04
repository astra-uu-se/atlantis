#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {
/*
bool int_le(FznInvariantGraph&, VarNodeId, VarNodeId);

bool int_le(FznInvariantGraph&, VarNodeId, Int, const fznparser::BoolArg& reif);

bool int_le(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::IntArg& reified);

bool int_le(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&);

bool int_le(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&,
            const fznparser::IntArg& reified);

bool int_le(FznInvariantGraph&, VarNodeId, fznparser::IntVar);

bool int_le(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_le(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::IntArg& reified);
*/
bool int_le(FznInvariantGraph&, VarNodeId, VarNodeId);
bool int_le(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_le(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn