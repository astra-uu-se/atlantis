#include "invariantgraph/constraints/intLinEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::IntLinEqNode>
invariantgraph::IntLinEqNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_lin_eq" && constraint.arguments.size() == 3) ||
      (constraint.name == "int_lin_eq_reif" &&
       constraint.arguments.size() == 4));

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  if (constraint.arguments.size() >= 4) {
    if (std::holds_alternative<bool>(constraint.arguments[3])) {
      auto shouldHold = std::get<bool>(constraint.arguments[3]);
      return std::make_unique<IntLinEqNode>(coeffs, variables, bound,
                                            shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[3], variableMap);
      return std::make_unique<IntLinEqNode>(coeffs, variables, bound, r);
    }
  }
  return std::make_unique<IntLinEqNode>(coeffs, variables, bound, true);
}

void invariantgraph::IntLinEqNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualView>(_sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualView>(_sumVarId, _c));
    }
  }
}

void invariantgraph::IntLinEqNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(
      staticInputs().begin(), staticInputs().end(),
      std::back_inserter(variables), [&](auto node) {
        if (node->variable() &&
            std::holds_alternative<fznparser::IntVariable>(*node->variable())) {
          return node->varId();
        }

        return engine.makeIntView<Bool2IntView>(node->varId());
      });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(_coeffs, variables, _sumVarId);
}