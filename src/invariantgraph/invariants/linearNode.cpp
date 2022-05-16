#include "invariantgraph/invariants/linearNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/linear.hpp"
#include "views/intOffsetView.hpp"

std::unique_ptr<invariantgraph::LinearNode>
invariantgraph::LinearNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.arguments.size() == 3);

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

  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(coeffs[definedVarIndex]) != 1) {
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

  auto linearInv =
      std::make_unique<invariantgraph::LinearNode>(coeffs, vars, output, -sum);

  return linearInv;
}

void invariantgraph::LinearNode::createDefinedVariables(Engine& engine) {
  if (staticInputs().size() == 1 &&
      staticInputs().front()->varId() != NULL_ID) {
    if (_coeffs.front() == 1 && _offset == 0) {
      definedVariables().front()->setVarId(staticInputs().front()->varId());
      return;
    }

    definedVariables().front()->setVarId(engine.makeIntView<IntOffsetView>(
        staticInputs().front()->varId(), _coeffs.front() * _offset));
    return;
  }

  if (_intermediateVarId == NULL_ID) {
    _intermediateVarId = engine.makeIntVar(0, 0, 0);
    assert(definedVariables().front()->varId() == NULL_ID);
    definedVariables().front()->setVarId(
        _offset == 0
            ? _intermediateVarId
            : engine.makeIntView<IntOffsetView>(_intermediateVarId, _offset));
  }
}

void invariantgraph::LinearNode::registerWithEngine(Engine& engine) {
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
