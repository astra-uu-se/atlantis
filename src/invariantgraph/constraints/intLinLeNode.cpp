#include "invariantgraph/constraints/intLinLeNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "constraints/lessEqual.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::IntLinLeNode>
invariantgraph::IntLinLeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_le");
  assert(constraint.arguments.size() == 3);

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  return std::make_unique<IntLinLeNode>(coeffs, variables, bound);
}

void invariantgraph::IntLinLeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  auto [sumLb, sumUb] = getDomainBounds();
  auto sumVar = engine.makeIntVar(0, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violation = registerViolation(engine, variableMap);
  auto bound = engine.makeIntVar(_bound, _bound, _bound);
  engine.makeConstraint<LessEqual>(violation, sumVar, bound);
}

std::pair<Int, Int> invariantgraph::IntLinLeNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    const auto& [varLb, varUb] = _variables[idx]->bounds();

    lb += _coeffs[idx] * varLb;
    ub += _coeffs[idx] * varUb;
  }

  return {lb, ub};
}
