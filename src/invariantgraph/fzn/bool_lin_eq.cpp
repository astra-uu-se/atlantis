#include "atlantis/invariantgraph/fzn/bool_lin_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"

namespace atlantis::invariantgraph::fzn {

static void verifyInputs(
    const std::vector<Int>& coeffs,
    const std::shared_ptr<fznparser::BoolVarArray>& inputs) {
  if (coeffs.size() != inputs->size()) {
    throw FznArgumentException(
        "bool_lin_eq constraint first and second array arguments must have the "
        "same length");
  }
}

bool bool_lin_eq(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                 const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                 Int bound) {
  verifyInputs(coeffs, inputs);
  graph.addInvariantNode(std::make_shared<BoolLinEqNode>(
      graph, std::move(coeffs), graph.retrieveVarNodes(inputs), bound));
  return true;
}

bool bool_lin_eq(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                 const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                 const std::shared_ptr<const fznparser::IntVar>& output) {
  verifyInputs(coeffs, inputs);
  graph.addInvariantNode(std::make_shared<BoolLinearNode>(
      graph, std::move(coeffs), graph.retrieveVarNodes(inputs),
      graph.retrieveVarNode(output)));
  return true;
}

bool bool_lin_eq(FznInvariantGraph& graph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lin_eq") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  std::vector<Int> coeffs =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0))
          ->toParVector();

  const auto bound = std::get<fznparser::IntArg>(constraint.arguments().at(2));
  if (bound.isFixed()) {
    return bool_lin_eq(
        graph, std::move(coeffs),
        getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)),
        bound.toParameter());
  }

  return bool_lin_eq(
      graph, std::move(coeffs),
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      bound.var());
}

}  // namespace atlantis::invariantgraph::fzn
