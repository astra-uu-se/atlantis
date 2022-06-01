#include "invariantgraph/constraints/globalCardinalityLowUpClosedNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::GlobalCardinalityLowUpClosedNode>
invariantgraph::GlobalCardinalityLowUpClosedNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto x = mappedVariableVector(model, constraint.arguments[0], variableMap);

  auto cover = integerVector(model, constraint.arguments[1]);

  auto low = integerVector(model, constraint.arguments[2]);

  auto up = integerVector(model, constraint.arguments[3]);

  assert(cover.size() == low.size());
  assert(cover.size() == up.size());

  bool shouldHold = true;
  VariableNode* r = nullptr;

  if (constraint.arguments.size() >= 5) {
    if (std::holds_alternative<bool>(constraint.arguments[4])) {
      shouldHold = std::get<bool>(constraint.arguments[4]);
    } else {
      r = mappedVariable(constraint.arguments[4], variableMap);
    }
  }

  if (r != nullptr) {
    return std::make_unique<GlobalCardinalityLowUpClosedNode>(x, cover, low, up,
                                                              r);
  }
  assert(r == nullptr);
  return std::make_unique<GlobalCardinalityLowUpClosedNode>(x, cover, low, up,
                                                            shouldHold);
}

void invariantgraph::GlobalCardinalityLowUpClosedNode::createDefinedVariables(
    Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (!shouldHold()) {
      _intermediate = engine.makeIntVar(0, 0, staticInputs().size());
      setViolationVarId(engine.makeIntView<NotEqualConst>(_intermediate, 0));
    } else {
      registerViolation(engine);
    }
  }
}

void invariantgraph::GlobalCardinalityLowUpClosedNode::registerWithEngine(
    Engine& engine) {
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](auto node) { return node->varId(); });

  if (shouldHold()) {
    engine.makeInvariant<GlobalCardinalityConst<true>>(violationVarId(), inputs,
                                                       _cover, _low, _up);
  } else {
    engine.makeInvariant<GlobalCardinalityConst<true>>(_intermediate, inputs,
                                                       _cover, _low, _up);
  }
}
