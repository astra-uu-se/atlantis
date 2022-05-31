#include "invariantgraph/constraints/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayBoolAndNode>
invariantgraph::ArrayBoolAndNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (std::holds_alternative<bool>(constraint.arguments[1])) {
    auto shouldHold = std::get<bool>(constraint.arguments[1]);
    return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, shouldHold);
  } else {
    auto r = mappedVariable(constraint.arguments[1], variableMap);
    return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, r);
  }
  return std::make_unique<invariantgraph::ArrayBoolAndNode>(as, true);
}

void invariantgraph::ArrayBoolAndNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (shouldHold()) {
      registerViolation(engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(engine.makeIntView<NotEqualConst>(_intermediate, 0));
    }
  }
}

void invariantgraph::ArrayBoolAndNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<ForAll>(!shouldHold() ? _intermediate : violationVarId(),
                               inputs);
}
