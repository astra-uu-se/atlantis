#include "invariantgraph/constraints/linNeNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/notEqual.hpp"
#include "invariants/linear.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::LinNeNode>
invariantgraph::LinNeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_lin_ne" && constraint.arguments.size() == 3) ||
      (constraint.name == "int_lin_ne_reif" &&
       constraint.arguments.size() == 4));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = integerValue(model, constraint.arguments[2]);

  VariableNode* r = constraint.arguments.size() >= 4
                        ? mappedVariable(constraint.arguments[3], variableMap)
                        : nullptr;

  return std::make_unique<LinNeNode>(coeffs, variables, c, r);
}

void invariantgraph::LinNeNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(!variableMap.contains(violation()));
    variableMap.emplace(violation(),
                        engine.makeIntView<NotEqualView>(_sumVarId, _c));
  }
}

void invariantgraph::LinNeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });

  assert(_sumVarId != NULL_ID);
  assert(variableMap.contains(violation()));

  engine.makeInvariant<Linear>(_coeffs, variables, _sumVarId);
}
