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

void invariantgraph::LinearNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return variableMap.at(node); });

  VarId intermediate = variables[0];

  // If there is only one variable in the input, there is no need to use a
  // linear invariant.
  if (variables.size() > 1 && _coeffs[0] == 1) {
    const auto& [lb, ub] = getIntermediateDomain();
    intermediate = engine.makeIntVar(lb, lb, ub);
    engine.makeInvariant<::Linear>(_coeffs, variables, intermediate);
  }

  auto outputVar = engine.makeIntView<IntOffsetView>(intermediate, _offset);
  variableMap.emplace(definedVariables()[0], outputVar);
}

std::pair<Int, Int> invariantgraph::LinearNode::getIntermediateDomain() const {
  Int lb = 0, ub = 0;

  for (size_t i = 0; i < _coeffs.size(); i++) {
    const auto& [varLb, varUb] = _variables[i]->bounds();

    lb += _coeffs[i] * varLb;
    ub += _coeffs[i] * varUb;
  }

  return {lb, ub};
}
