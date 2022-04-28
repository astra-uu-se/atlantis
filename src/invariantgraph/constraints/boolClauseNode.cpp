#include "invariantgraph/constraints/boolClauseNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

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

void invariantgraph::BoolClauseNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(!variableMap.contains(violation()));
    variableMap.emplace(violation(),
                        engine.makeIntView<EqualView>(
                            _sumVarId, static_cast<Int>(_as.size()) +
                                           static_cast<Int>(_bs.size())));
  }
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
                   return engine.makeIntView<NotEqualView>(b, 0);
                 });

  assert(_sumVarId != NULL_ID);
  assert(variableMap.contains(violation()));
  engine.makeInvariant<Linear>(engineVariables, _sumVarId);
}
