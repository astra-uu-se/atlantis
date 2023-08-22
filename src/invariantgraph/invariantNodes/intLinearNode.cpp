#include "invariantgraph/invariantNodes/intLinearNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntLinearNode> IntLinearNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  const fznparser::IntVarArray& vars =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  // The negative sum is the offset of the defined variable:
  auto sum =
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).parameter();

  const std::optional<std::reference_wrapper<const fznparser::Variable>>
      definedVariableRef = constraint.definedVariable();

  assert(definedVariableRef.has_value());

  const fznparser::IntVar& definedVariable =
      std::get<fznparser::IntVar>(definedVariableRef.value().get());

  size_t definedVarIndex = vars.size();
  for (size_t i = 0; i < vars.size(); ++i) {
    if (!std::holds_alternative<Int>(vars.at(i)) &&
        std::get<std::reference_wrapper<const fznparser::IntVar>>(vars.at(i))
                .get()
                .identifier() == definedVariable.identifier()) {
      definedVarIndex = i;
      break;
    }
  }

  assert(definedVarIndex < vars.size());

  Int definedVarCoeff = coeffs.at(definedVarIndex);

  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(definedVarCoeff) != 1) {
    throw std::runtime_error(
        "Cannot define variable with coefficient which is not +/-1");
  }

  auto coeffsIt = coeffs.begin();
  std::advance(coeffsIt, definedVarIndex);
  coeffs.erase(coeffsIt);

  std::vector<VarNodeId> addedVars = invariantGraph.createVarNodes(vars);
  auto varsIt = addedVars.begin();
  std::advance(varsIt, definedVarIndex);
  auto output = *varsIt;
  addedVars.erase(varsIt);

  return std::make_unique<IntLinearNode>(
      std::move(coeffs), std::move(addedVars), output, definedVarCoeff, sum);
}

void IntLinearNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                            Engine& engine) {
  if (staticInputVarNodeIds().size() == 1 &&
      staticInputVarNodeIds().front()->varId() != NULL_ID) {
    if (_coeffs.front() == 1 && _sum == 0) {
      outputVarNodeIds().front()->setVarId(
          staticInputVarNodeIds().front()->varId());
      return;
    }

    if (_definingCoefficient == -1) {
      auto scalar = engine.makeIntView<ScalarView>(
          engine, staticInputVarNodeIds().front()->varId(), _coeffs.front());
      outputVarNodeIds().front()->setVarId(
          engine.makeIntView<IntOffsetView>(engine, scalar, -_sum));
    } else {
      assert(_definingCoefficient == 1);
      auto scalar = engine.makeIntView<ScalarView>(
          engine, staticInputVarNodeIds().front()->varId(), -_coeffs.front());
      outputVarNodeIds().front()->setVarId(
          engine.makeIntView<IntOffsetView>(engine, scalar, _sum));
    }

    return;
  }

  if (_intermediateVarId == NULL_ID) {
    _intermediateVarId = engine.makeIntVar(0, 0, 0);
    assert(outputVarNodeIds().front()->varId() == NULL_ID);

    auto offsetIntermediate = _intermediateVarId;
    if (_sum != 0) {
      offsetIntermediate =
          engine.makeIntView<IntOffsetView>(engine, _intermediateVarId, -_sum);
    }

    auto invertedIntermediate = offsetIntermediate;
    if (_definingCoefficient == 1) {
      invertedIntermediate =
          engine.makeIntView<ScalarView>(engine, offsetIntermediate, -1);
    }

    outputVarNodeIds().front()->setVarId(invertedIntermediate);
  }
}

void IntLinearNode::registerNode(InvariantGraph& invariantGraph,
                                 Engine& engine) {
  assert(outputVarNodeIds().front()->varId() != NULL_ID);

  std::vector<VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->varId(); });
  if (_intermediateVarId == NULL_ID) {
    assert(variables.size() == 1);
    return;
  }

  engine.makeInvariant<Linear>(engine, _intermediateVarId, _coeffs, variables);
}

}  // namespace invariantgraph