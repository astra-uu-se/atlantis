#include "invariantgraph/constraints/allDifferentNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/allDifferent.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::AllDifferentNode>
invariantgraph::AllDifferentNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "fzn_all_different_int" && constraint.arguments.size() == 1) ||
      (constraint.name == "fzn_all_different_int_reif" &&
       constraint.arguments.size() == 2));

  auto variables =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (constraint.arguments.size() >= 2) {
    if (std::holds_alternative<bool>(constraint.arguments[1])) {
      auto shouldHold = std::get<bool>(constraint.arguments[1]);
      return std::make_unique<invariantgraph::AllDifferentNode>(variables,
                                                                shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[1], variableMap);
      return std::make_unique<invariantgraph::AllDifferentNode>(variables, r);
    }
  }
  return std::make_unique<AllDifferentNode>(variables, true);
}

void invariantgraph::AllDifferentNode::createDefinedVariables(Engine& engine) {
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

void invariantgraph::AllDifferentNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  engine.makeConstraint<AllDifferent>(
      !shouldHold() ? _intermediate : violationVarId(), engineVariables);
}
