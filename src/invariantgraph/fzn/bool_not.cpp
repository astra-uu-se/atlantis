#include "atlantis/invariantgraph/fzn/bool_not.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_not(FznInvariantGraph& graph, const fznparser::BoolArg& boolVar,
              const fznparser::BoolArg& negatedBoolVar) {
  graph.addInvariantNode(
      std::make_shared<BoolNotNode>(graph, graph.retrieveVarNode(boolVar),
                                    graph.retrieveVarNode(negatedBoolVar)));
  return true;
}

bool bool_not(FznInvariantGraph& graph,
              const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_not") {
    return false;
  }
  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)

  return bool_not(graph,
                  std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                  std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
