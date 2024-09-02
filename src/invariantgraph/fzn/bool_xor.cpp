#include "atlantis/invariantgraph/fzn/bool_xor.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/bool_eq.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_xor(FznInvariantGraph& graph, const fznparser::BoolArg& a,
              const fznparser::BoolArg& b) {
  graph.addInvariantNode(std::make_shared<ArrayBoolXorNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b)));
  return true;
}

bool bool_xor(FznInvariantGraph& graph, const fznparser::BoolArg& a,
              const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<ArrayBoolXorNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(reified)));

  return true;
}

bool bool_xor(FznInvariantGraph& graph,
              const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_xor") {
    return false;
  }

  bool isReified = constraint.arguments().size() >= 3;
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)

  if (!isReified) {
    return bool_xor(graph,
                    std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                    std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return bool_xor(graph,
                  std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                  std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                  std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
