#include "invariantgraph/invariants/arrayBoolOrNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/exists.hpp"
#include "views/violation2BoolView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolOrNode>
invariantgraph::ArrayBoolOrNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_or");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  auto r = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::ArrayBoolOrNode>(as, r);
}

void invariantgraph::ArrayBoolOrNode::createDefinedVariables(Engine& engine) {
  if (_violationVarId == NULL_ID) {
    _violationVarId = engine.makeIntVar(0, 0, 0);
    assert(definedVariables().front()->varId() == NULL_ID);
    definedVariables().front()->setVarId(
        engine.makeIntView<Violation2BoolView>(_violationVarId));
  }
}

void invariantgraph::ArrayBoolOrNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });
#ifndef NDEBUG
  for (const VarId input : inputs) {
    assert(0 <= engine.lowerBound(input));
  }
#endif
  engine.makeInvariant<Exists>(inputs, _violationVarId);
}
