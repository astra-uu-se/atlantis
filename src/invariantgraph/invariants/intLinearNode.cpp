#include "invariantgraph/invariants/intLinearNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/linear.hpp"
#include "views/intOffsetView.hpp"
#include "views/scalarView.hpp"

std::unique_ptr<invariantgraph::IntLinearNode>
invariantgraph::IntLinearNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto vars = mappedVariableVector(model, constraint.arguments[1], variableMap);
  // The negative sum is the offset of the defined variable:
  auto sum = integerValue(model, constraint.arguments[2]);

  auto definedVarId = definedVariable(constraint);
  assert(definedVarId);

  auto definedVarPos =
      std::find_if(vars.begin(), vars.end(), [&](auto varNode) {
        assert(varNode->variable());

        return std::visit<bool>(
            [&](const auto& var) { return definedVarId == var.name; },
            *varNode->variable());
      });

  assert(definedVarPos != vars.end());
  size_t definedVarIndex = definedVarPos - vars.begin();
  auto definedVarCoeff = coeffs[definedVarIndex];

  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(definedVarCoeff) != 1) {
    throw std::runtime_error(
        "Cannot define variable with coefficient which is not +/-1");
  }

  auto coeffsIt = coeffs.begin();
  std::advance(coeffsIt, definedVarIndex);
  coeffs.erase(coeffsIt);

  auto varsIt = vars.begin();
  std::advance(varsIt, definedVarIndex);
  auto output = *varsIt;
  vars.erase(varsIt);

  auto linearInv = std::make_unique<invariantgraph::IntLinearNode>(
      coeffs, vars, output, definedVarCoeff, sum);

  return linearInv;
}

void invariantgraph::IntLinearNode::createDefinedVariables(Engine& engine) {
  if (staticInputs().size() == 1 &&
      staticInputs().front()->varId() != NULL_ID) {
    if (_coeffs.front() == 1 && _sum == 0) {
      definedVariables().front()->setVarId(staticInputs().front()->varId());
      return;
    }

    if (_definingCoefficient == -1) {
      auto scalar = engine.makeIntView<ScalarView>(
          staticInputs().front()->varId(), _coeffs.front());
      definedVariables().front()->setVarId(
          engine.makeIntView<IntOffsetView>(scalar, -_sum));
    } else {
      assert(_definingCoefficient == 1);
      auto scalar = engine.makeIntView<ScalarView>(
          staticInputs().front()->varId(), -_coeffs.front());
      definedVariables().front()->setVarId(
          engine.makeIntView<IntOffsetView>(scalar, _sum));
    }

    return;
  }

  if (_intermediateVarId == NULL_ID) {
    _intermediateVarId = engine.makeIntVar(0, 0, 0);
    assert(definedVariables().front()->varId() == NULL_ID);

    auto offsetIntermediate = _intermediateVarId;
    if (_sum != 0) {
      offsetIntermediate =
          engine.makeIntView<IntOffsetView>(_intermediateVarId, -_sum);
    }

    auto invertedIntermediate = offsetIntermediate;
    if (_definingCoefficient == 1) {
      invertedIntermediate =
          engine.makeIntView<ScalarView>(offsetIntermediate, -1);
    }

    definedVariables().front()->setVarId(invertedIntermediate);
  }
}

void invariantgraph::IntLinearNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);

  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->varId(); });
  if (_intermediateVarId == NULL_ID) {
    assert(variables.size() == 1);
    return;
  }

  engine.makeInvariant<::Linear>(_coeffs, variables, _intermediateVarId);
}
