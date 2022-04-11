#include "invariantgraph/constraints/intLinNeNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/linear.hpp"
#include "constraints/notEqual.hpp"

std::unique_ptr<invariantgraph::IntLinNeNode>
invariantgraph::IntLinNeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_ne");
  assert(constraint.arguments.size() == 3);

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(constraint.arguments[1], variableMap);
  auto c = integerValue(model, constraint.arguments[2]);

  return std::make_unique<IntLinNeNode>(coeffs, variables, c);
}

void invariantgraph::IntLinNeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  auto [sumLb, sumUb] = getLinearDomainBounds();
  auto sumVar = engine.makeIntVar(0, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violation = registerViolation(engine, variableMap);
  auto c = engine.makeIntVar(_c, _c, _c);
  engine.makeConstraint<NotEqual>(violation, sumVar, c);
}

std::pair<Int, Int> invariantgraph::IntLinNeNode::getLinearDomainBounds()
    const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    const auto& [varLb, varUb] = _variables[idx]->bounds();

    lb += _coeffs[idx] * varLb;
    ub += _coeffs[idx] * varUb;
  }

  return {lb, ub};
}
