#include "invariantgraph/constraints/intLinNeNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/notEqual.hpp"
#include "invariants/linear.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::IntLinNeNode>
invariantgraph::IntLinNeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_ne");
  assert(constraint.arguments.size() == 3);

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = integerValue(model, constraint.arguments[2]);

  return std::make_unique<IntLinNeNode>(coeffs, variables, c);
}

void invariantgraph::IntLinNeNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(!variableMap.contains(violation()));
    variableMap.emplace(violation(),
                        engine.makeIntView<NotEqualView>(_sumVarId, _c));
  }
}

void invariantgraph::IntLinNeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });

  assert(_sumVarId != NULL_ID);
  assert(variableMap.contains(violation()));

  engine.makeInvariant<Linear>(_coeffs, variables, _sumVarId);
}
