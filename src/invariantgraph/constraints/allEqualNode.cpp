#include "invariantgraph/constraints/allEqualNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/allDifferent.hpp"

std::unique_ptr<invariantgraph::AllEqualNode>
invariantgraph::AllEqualNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto variables =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (constraint.arguments.size() >= 2) {
    if (std::holds_alternative<bool>(constraint.arguments[1])) {
      auto shouldHold = std::get<bool>(constraint.arguments[1]);
      return std::make_unique<invariantgraph::AllEqualNode>(variables,
                                                            shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[1], variableMap);
      return std::make_unique<invariantgraph::AllEqualNode>(variables, r);
    }
  }
  return std::make_unique<AllEqualNode>(variables, true);
}

void invariantgraph::AllEqualNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _allDifferentViolationVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualConst>(
          _allDifferentViolationVarId, staticInputs().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualConst>(
          _allDifferentViolationVarId, staticInputs().size() - 1));
    }
  }
}

void invariantgraph::AllEqualNode::registerWithEngine(Engine& engine) {
  assert(_allDifferentViolationVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  std::vector<VarId> engineVariables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  engine.makeConstraint<AllDifferent>(_allDifferentViolationVarId,
                                      engineVariables);
}
