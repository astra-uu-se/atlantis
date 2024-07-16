#include "atlantis/invariantgraph/fzn/int_div.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_div(FznInvariantGraph& graph, const fznparser::IntArg& numerator,
             const fznparser::IntArg& denominator,
             const fznparser::IntArg& quotient) {
  graph.addInvariantNode(std::make_unique<IntDivNode>(
      graph.retrieveVarNode(numerator), graph.retrieveVarNode(denominator),
      graph.retrieveVarNode(quotient)));
  return true;
}

bool int_div(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_div") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)

  return int_div(graph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
