#include "atlantis/invariantgraph/fzn/int_pow.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_pow(FznInvariantGraph& graph, const fznparser::IntArg& base,
             const fznparser::IntArg& exponent,
             const fznparser::IntArg& power) {
  graph.addInvariantNode(std::make_unique<IntPowNode>(
      graph.retrieveVarNode(base), graph.retrieveVarNode(exponent),
      graph.retrieveVarNode(power)));
  return true;
}

bool int_pow(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_pow") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)

  return int_pow(graph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
