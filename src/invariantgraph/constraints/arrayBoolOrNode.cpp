#include "invariantgraph/constraints/arrayBoolOrNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/exists.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolOrNode>
invariantgraph::ArrayBoolOrNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_or" &&
         constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (std::holds_alternative<bool>(constraint.arguments[1])) {
    auto value = std::get<bool>(constraint.arguments[1]);
    return std::make_unique<invariantgraph::ArrayBoolOrNode>(as, value);
  } else {
    auto r = mappedVariable(constraint.arguments[1], variableMap);
    return std::make_unique<invariantgraph::ArrayBoolOrNode>(as, r);
  }
}

void invariantgraph::ArrayBoolOrNode::createDefinedVariables(Engine& engine) {
  if (_rIsConstant && !_rValue) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    setViolationVarId(engine.makeIntView<NotEqualView>(_sumVarId, 0));
  } else {
    registerViolation(engine);
  }
}

void invariantgraph::ArrayBoolOrNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<Exists>(
      inputs, _rIsConstant && !_rValue ? _sumVarId : violationVarId());
}
