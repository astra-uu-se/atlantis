#include "invariantgraph/constraints/arrayBoolXorNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayBoolXorNode>
invariantgraph::ArrayBoolXorNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (std::holds_alternative<bool>(constraint.arguments[1])) {
    auto shouldHold = std::get<bool>(constraint.arguments[1]);
    return std::make_unique<invariantgraph::ArrayBoolXorNode>(as, shouldHold);
  } else {
    auto r = mappedVariable(constraint.arguments[1], variableMap);
    return std::make_unique<invariantgraph::ArrayBoolXorNode>(as, r);
  }
  return std::make_unique<invariantgraph::ArrayBoolXorNode>(as, true);
}

void invariantgraph::ArrayBoolXorNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualView>(_intermediate, 1));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualView>(_intermediate, 1));
    }
  }
}

void invariantgraph::ArrayBoolXorNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<BoolLinear>(inputs, _intermediate);
}
