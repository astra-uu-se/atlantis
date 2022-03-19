#include "invariantgraph/constraints/intLinEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::IntLinEqNode>
invariantgraph::IntLinEqNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_eq");
  assert(constraint.arguments.size() == 3);

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  return std::make_unique<IntLinEqNode>(coeffs, variables, bound);
}

void invariantgraph::IntLinEqNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  auto [sumLb, sumUb] = getDomainBounds();
  auto sumVar = engine.makeIntVar(sumLb, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violationVar = registerViolation(engine, variableMap);
  auto c = engine.makeIntVar(_c, _c, _c);
  engine.makeConstraint<Equal>(violationVar, sumVar, c);
}

std::pair<Int, Int> invariantgraph::IntLinEqNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    const auto& [varLb, varUb] = _variables[idx]->bounds();

    lb += _coeffs[idx] * varLb;
    ub += _coeffs[idx] * varUb;
  }

  return {lb, ub};
}
