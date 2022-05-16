#include "invariantgraph/constraints/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/elementConst.hpp"
#include "invariants/forAll.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolAndNode>
invariantgraph::ArrayBoolAndNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_and" &&
         constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  if (std::holds_alternative<bool>(constraint.arguments[1])) {
    auto value = std::get<bool>(constraint.arguments[1]);
    return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, value);
  } else {
    auto r = mappedVariable(constraint.arguments[1], variableMap);
    return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, r);
  }
}

void invariantgraph::ArrayBoolAndNode::createDefinedVariables(Engine& engine) {
  if (_rIsConstant && !_rValue) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    setViolationVarId(engine.makeIntView<NotEqualView>(_sumVarId, 0));
  } else {
    registerViolation(engine);
  }
}

void invariantgraph::ArrayBoolAndNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<ForAll>(
      inputs, _rIsConstant && !_rValue ? _sumVarId : violationVarId());
}
