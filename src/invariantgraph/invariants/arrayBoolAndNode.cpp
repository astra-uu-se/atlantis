#include "invariantgraph/invariants/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementConst.hpp"
#include "invariants/forAll.hpp"
#include "views/violation2BoolView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolAndNode>
invariantgraph::ArrayBoolAndNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_and");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  auto r = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, r);
}

void invariantgraph::ArrayBoolAndNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    assert(!variableMap.contains(definedVariables()[0]));
    variableMap.emplace(definedVariables()[0],
                        engine.makeIntView<Violation2BoolView>(_sumVarId));
  }
}

void invariantgraph::ArrayBoolAndNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return variableMap.at(node); });

  engine.makeInvariant<ForAll>(inputs, _sumVarId);
}
