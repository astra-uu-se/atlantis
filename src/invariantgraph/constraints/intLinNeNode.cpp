#include "invariantgraph/constraints/intLinNeNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntLinNeNode>
invariantgraph::IntLinNeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = integerValue(model, constraint.arguments[2]);

  if (constraint.arguments.size() >= 4) {
    if (std::holds_alternative<bool>(constraint.arguments[3])) {
      auto shouldHold = std::get<bool>(constraint.arguments[3]);
      return std::make_unique<IntLinNeNode>(coeffs, variables, c, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[3], variableMap);
      return std::make_unique<IntLinNeNode>(coeffs, variables, c, r);
    }
  }
  return std::make_unique<IntLinNeNode>(coeffs, variables, c, true);
}

void invariantgraph::IntLinNeNode::createDefinedVariables(Engine& engine) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<NotEqualView>(_sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<EqualView>(_sumVarId, _c));
    }
  }
}

void invariantgraph::IntLinNeNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(_sumVarId, _coeffs, variables);
}
