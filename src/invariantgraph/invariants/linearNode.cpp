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

void invariantgraph::LinearNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (_variables.size() == 1 && variableMap.contains(_variables.front())) {
    if (_coeffs.front() == 1 && _offset == 1) {
      variableMap.emplace(definedVariables()[0],
                          variableMap.at(_variables.front()));
      return;
    }

    variableMap.emplace(
        definedVariables()[0],
        engine.makeIntView<IntOffsetView>(variableMap.at(_variables.front()),
                                          _coeffs.front() * _offset));
    return;
  }

  if (_intermediateVarId == NULL_ID) {
    _intermediateVarId = engine.makeIntVar(0, 0, 0);
    assert(!variableMap.contains(definedVariables()[0]));
    variableMap.emplace(definedVariables()[0],
                        _offset == 1 ? _intermediateVarId
                                     : engine.makeIntView<IntOffsetView>(
                                           _intermediateVarId, _offset));
  }
}

void invariantgraph::LinearNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(definedVariables()[0]));

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return variableMap.at(node); });
  if (_intermediateVarId == NULL_ID) {
    assert(variables.size() == 1);
    return;
  }

  engine.makeInvariant<::Linear>(_coeffs, variables, _intermediateVarId);
}
