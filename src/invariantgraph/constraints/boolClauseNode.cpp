#include "invariantgraph/constraints/boolClauseNode.hpp"

#include "constraints/lessThan.hpp"
#include "invariantgraph/parseHelper.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::BoolClauseNode>
invariantgraph::BoolClauseNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "bool_clause");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  auto bs = mappedVariableVector(model, constraint.arguments[1], variableMap);

  return std::make_unique<BoolClauseNode>(as, bs);
}

void invariantgraph::BoolClauseNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> engineVariables;
  engineVariables.reserve(_as.size() + _bs.size());
  std::transform(_as.begin(), _as.end(), std::back_inserter(engineVariables),
                 [&](const auto& var) { return variableMap.at(var); });

  std::transform(_bs.begin(), _bs.end(), std::back_inserter(engineVariables),
                 [&](const auto& var) {
                   auto b = variableMap.at(var);

                   // Bool2Int is functionally the same as negation, since it
                   // does 1 - b.
                   return engine.makeIntView<Bool2IntView>(b);
                 });

  auto sum = engine.makeIntVar(0, 0, 0);
  engine.makeInvariant<Linear>(engineVariables, sum);

  VarId violation = registerViolation(engine, variableMap);
  VarId constZero = engine.makeIntVar(0, 0, 0);
  engine.makeConstraint<LessThan>(violation, constZero, sum);
}
