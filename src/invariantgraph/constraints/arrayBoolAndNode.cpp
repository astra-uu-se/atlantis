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
      setViolationVarId(engine.makeIntView<NotEqualView>(_intermediate, 0));
    }
  }
}

void invariantgraph::ArrayBoolAndNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  std::vector<VarId> inputs;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(inputs),
                 [&](const auto& node) { return node->varId(); });

  engine.makeInvariant<ForAll>(
      inputs, !shouldHold() ? _intermediate : violationVarId());
}
