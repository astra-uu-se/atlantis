#include "invariantgraph/constraints/intLinLeNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntLinLeNode>
invariantgraph::IntLinLeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_lin_le" && constraint.arguments.size() == 3) ||
      (constraint.name == "int_lin_le_reif" &&
       constraint.arguments.size() == 4));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  if (constraint.arguments.size() >= 4) {
    if (std::holds_alternative<bool>(constraint.arguments[3])) {
      auto shouldHold = std::get<bool>(constraint.arguments[3]);
      return std::make_unique<IntLinLeNode>(coeffs, variables, bound,
                                            shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[3], variableMap);
      return std::make_unique<IntLinLeNode>(coeffs, variables, bound, r);
    }
  }
  return std::make_unique<IntLinLeNode>(coeffs, variables, bound, true);
}

void invariantgraph::IntLinLeNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<LessEqualView>(_sumVarId, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<GreaterThanView>(_sumVarId, _bound));
    }
  }
}

void invariantgraph::IntLinLeNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(_coeffs, variables, _sumVarId);
}
