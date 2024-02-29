#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"

namespace atlantis::invariantgraph::fzn {

/*


bool int_lt(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&);

bool int_lt(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&,
            const fznparser::BoolArg& reified);

bool int_lt(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::IntArg& reified);

bool int_lt(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&);

bool int_lt(FznInvariantGraph&, VarNodeId, const fznparser::IntArg&,
            const fznparser::IntArg& reified);

bool int_lt(FznInvariantGraph&, VarNodeId, fznparser::IntVar);

bool int_lt(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&);

bool int_lt(FznInvariantGraph&, const fznparser::IntArg&,
            const fznparser::IntArg&, const fznparser::IntArg& reified);
*/
bool int_lt(FznInvariantGraph&, Int, VarNodeId);
bool int_lt(FznInvariantGraph&, VarNodeId, Int);
bool int_lt(FznInvariantGraph&, VarNodeId, VarNodeId);
bool int_lt(FznInvariantGraph&, VarNodeId, VarNodeId,
            const fznparser::BoolArg& reified);

bool int_lt(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
