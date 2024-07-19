#include "atlantis/invariantgraph/fzn/int_max.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_max(FznInvariantGraph& graph, const fznparser::IntArg& a,
             const fznparser::IntArg& b, const fznparser::IntArg& maximum) {
  const VarNodeId outputVarNodeId = graph.retrieveVarNode(maximum);

  if (a.isFixed() && b.isFixed()) {
    graph.varNode(outputVarNodeId)
        .fixToValue(std::max(a.toParameter(), b.toParameter()));
    return true;
  }
  graph.addInvariantNode(std::make_unique<ArrayIntMaximumNode>(
      graph.retrieveVarNode(a), graph.retrieveVarNode(b), outputVarNodeId));
  return true;
}

bool int_max(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_max") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)

  return int_max(graph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
