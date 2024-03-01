#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {
/*
bool int_eq(FznInvariantGraph&, VarNodeId, Int);

bool int_eq(FznInvariantGraph&, VarNodeId, Int,
            const fznparser::IntArg& reified);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::IntArg& reified);

bool int_eq(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&);

bool int_eq(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&,
            const fznparser::IntArg& reified);

bool int_eq(FznInvariantGraph&, VarNodeId, fznparser::IntVar);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::IntArg& reified);
*/
bool int_eq(FznInvariantGraph&, VarNodeId, Int);

bool int_eq(FznInvariantGraph&, VarNodeId, Int,
            const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId, VarNodeId reified);

bool int_eq(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_eq(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::BoolArg& reified);

bool int_eq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
