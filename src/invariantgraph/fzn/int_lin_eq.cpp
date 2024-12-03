#include "atlantis/invariantgraph/fzn/int_lin_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLinEqNode.hpp"

namespace atlantis::invariantgraph::fzn {

static void verifyInputs(
    const std::vector<Int>& coeffs,
    const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  if (coeffs.size() != inputs->size()) {
    throw FznArgumentException(
        "int_lin_eq constraint first and second array arguments must have the "
        "same length");
  }
}

bool int_lin_eq(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound,
                const std::shared_ptr<const fznparser::IntVar>& definedVar) {
  verifyInputs(coeffs, inputs);
  Int definedVarCoeff = 0;
  std::vector<Int> definedVarIndices;
  for (Int i = static_cast<Int>(coeffs.size()) - 1; i >= 0; --i) {
    if (std::holds_alternative<std::shared_ptr<const fznparser::IntVar>>(
            inputs->at(i))) {
      const auto inputVar =
          std::get<std::shared_ptr<const fznparser::IntVar>>(inputs->at(i));
      if (inputVar == definedVar) {
        definedVarCoeff += coeffs.at(i);
        definedVarIndices.push_back(i);
      }
    }
  }

  if (definedVarCoeff == 0) {
    return int_lin_eq(graph, std::move(coeffs), inputs, bound);
  }

  if (std::abs(definedVarCoeff) != 1) {
    throw FznArgumentException(
        "int_lin_eq constraint defined variable must have a coefficient of 1 "
        "or -1");
  }

  std::vector<VarNodeId> inputVarNodes = graph.retrieveVarNodes(inputs);

  const VarNodeId definedVarNodeId =
      inputVarNodes.at(definedVarIndices.front());

  for (const Int index : definedVarIndices) {
    inputVarNodes.erase(inputVarNodes.begin() + index);
    coeffs.erase(coeffs.begin() + index);
  }

  if (definedVarCoeff == 1) {
    // swapping lhs and rhs, moving the defined variable to the rhs. Then
    // reducing both lhs and rhs by all other variables, making the defined
    // variable the only variable on the rhs.
    for (size_t i = 0; i < coeffs.size(); ++i) {
      coeffs.at(i) = -coeffs.at(i);
    }
  }  // otherwise (with definedVarCoeff = -1), add the defined variable to both
     // sides.

  // If the defined variable constant is -1, then the sides have not been
  // swapped, and the constant must be reduced from both sides:
  const Int lhsOffset = definedVarCoeff == 1 ? bound : -bound;

  graph.addInvariantNode(std::make_shared<IntLinearNode>(
      graph, std::move(coeffs), std::move(inputVarNodes), definedVarNodeId,
      lhsOffset));
  return true;
}

bool int_lin_eq(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound,
                const std::shared_ptr<const fznparser::IntVar>& definedVar,
                fznparser::BoolArg reified) {
  verifyInputs(coeffs, inputs);
  Int definedVarCoeff = 0;
  std::vector<Int> definedVarIndices;
  for (Int i = static_cast<Int>(coeffs.size()) - 1; i >= 0; --i) {
    if (std::holds_alternative<std::shared_ptr<const fznparser::IntVar>>(
            inputs->at(i))) {
      const auto inputVar =
          std::get<std::shared_ptr<const fznparser::IntVar>>(inputs->at(i));
      if (inputVar == definedVar) {
        definedVarCoeff += coeffs.at(i);
        definedVarIndices.push_back(i);
      }
    }
  }
  if (definedVarCoeff == 0) {
    return int_lin_eq(graph, std::move(coeffs), inputs, bound, reified);
  }
  if (std::abs(definedVarCoeff) != 1) {
    throw FznArgumentException(
        "int_lin_eq constraint defined variable must have a coefficient of 1 "
        "or -1");
  }
  std::vector<VarNodeId> inputVarNodes = graph.retrieveVarNodes(inputs);

  const VarNodeId definedVarNodeId =
      inputVarNodes.at(definedVarIndices.front());

  for (const Int index : definedVarIndices) {
    inputVarNodes.erase(inputVarNodes.begin() + index);
    coeffs.erase(coeffs.begin() + index);
  }

  if (definedVarCoeff == 1) {
    for (size_t i = 0; i < coeffs.size(); ++i) {
      coeffs.at(i) = -coeffs.at(i);
    }
  }

  // If the defined variable constant is -1, then the sides have not been
  // swapped, and the constant must be reduced from both sides:
  const Int lhsOffset = definedVarCoeff == 1 ? bound : -bound;
  auto [lb, ub] = linBounds(coeffs, inputs);
  lb += lhsOffset;
  ub += lhsOffset;
  const VarNodeId outputVarNodeId =
      graph.retrieveIntVarNode(SearchDomain(lb, ub), VarNode::DomainType::NONE);

  graph.addInvariantNode(std::make_shared<IntLinearNode>(
      graph, std::move(coeffs), std::move(inputVarNodes), outputVarNodeId,
      lhsOffset));
  graph.addInvariantNode(std::make_shared<IntAllEqualNode>(
      graph, outputVarNodeId, definedVarNodeId,
      graph.retrieveVarNode(reified)));
  return true;
}

bool int_lin_eq(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound) {
  verifyInputs(coeffs, inputs);
  if (coeffs.empty()) {
    if (bound == 0) {
      return true;
    }
    throw FznArgumentException(
        "int_lin_eq constraint with empty arrays must have a total sum of 0");
  }

  graph.addInvariantNode(std::make_shared<IntLinEqNode>(
      graph, std::move(coeffs), graph.retrieveVarNodes(inputs), bound));

  return true;
}

bool int_lin_eq(FznInvariantGraph& graph, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound, fznparser::BoolArg reified) {
  verifyInputs(coeffs, inputs);

  graph.addInvariantNode(std::make_shared<IntLinEqNode>(
      graph, std::move(coeffs), graph.retrieveVarNodes(inputs), bound,
      graph.retrieveVarNode(reified)));

  return true;
}

bool int_lin_eq(FznInvariantGraph& graph,
                const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_lin_eq" &&
      constraint.identifier() != "int_lin_eq_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)
  if (isReified) {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  }

  std::vector<Int> coeffs =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0))
          ->toParVector();

  if (constraint.definedVar().has_value() &&
      std::holds_alternative<std::shared_ptr<fznparser::IntVar>>(
          constraint.definedVar().value())) {
    const std::shared_ptr<const fznparser::IntVar> definedVar =
        std::get<std::shared_ptr<fznparser::IntVar>>(
            constraint.definedVar().value());
    if (!isReified) {
      return int_lin_eq(
          graph, std::move(coeffs),
          getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
          std::get<fznparser::IntArg>(constraint.arguments().at(2))
              .toParameter(),
          definedVar);
    } else {
      return int_lin_eq(
          graph, std::move(coeffs),
          getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
          std::get<fznparser::IntArg>(constraint.arguments().at(2))
              .toParameter(),
          definedVar,
          std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
    }
  } else if (!isReified) {
    return int_lin_eq(
        graph, std::move(coeffs),
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2))
            .toParameter());
  } else {
    return int_lin_eq(
        graph, std::move(coeffs),
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter(),
        std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
  }
}

}  // namespace atlantis::invariantgraph::fzn
