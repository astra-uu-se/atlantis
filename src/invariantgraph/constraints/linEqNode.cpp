#include "invariantgraph/constraints/linEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::LinEqNode>
invariantgraph::LinEqNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_eq" || constraint.name == "bool_lin_eq");
  assert(constraint.arguments.size() == 3);

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  return std::make_unique<LinEqNode>(coeffs, variables, bound);
}

void invariantgraph::LinEqNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> variables;
  std::transform(
      _variables.begin(), _variables.end(), std::back_inserter(variables),
      [&](auto node) {
        if (node->variable() &&
            std::holds_alternative<fznparser::IntVariable>(*node->variable())) {
          return variableMap.at(node);
        }

        return engine.makeIntView<Bool2IntView>(variableMap.at(node));
      });

  auto sumVar = engine.makeIntVar(0, 0, 0);
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violationVar = registerViolation(engine, variableMap);
  auto c = engine.makeIntVar(_c, _c, _c);
  engine.makeConstraint<Equal>(violationVar, sumVar, c);
}
