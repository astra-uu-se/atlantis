#include "atlantis/invariantgraph/fzn/bool_lin_le.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_le(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                 const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                 Int bound) {
  graph.addInvariantNode(std::make_unique<BoolLinLeNode>(
      std::move(coeffs), graph.retrieveVarNodes(inputs), bound));
  return true;
}

bool bool_lin_le(FznInvariantGraph& graph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lin_le") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  std::vector<Int> coeffs =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0))
          ->toParVector();

  return bool_lin_le(
      graph, std::move(coeffs),
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn
