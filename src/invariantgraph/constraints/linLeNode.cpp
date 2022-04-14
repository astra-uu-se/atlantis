#include "invariantgraph/constraints/linLeNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "constraints/lessEqual.hpp"
#include "invariants/linear.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::LinLeNode>
invariantgraph::LinLeNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_le" || constraint.name == "bool_lin_le");
  assert(constraint.arguments.size() == 3);

  auto coeffs = integerVector(model, constraint.arguments[0]);
  auto variables =
      mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto bound = integerValue(model, constraint.arguments[2]);

  return std::make_unique<LinLeNode>(coeffs, variables, bound);
}

void invariantgraph::LinLeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  auto sumVar = engine.makeIntVar(0, 0, 0);

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
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violation = registerViolation(engine, variableMap);
  auto bound = engine.makeIntVar(_bound, _bound, _bound);
  engine.makeConstraint<LessEqual>(violation, sumVar, bound);
}

std::pair<Int, Int> invariantgraph::LinLeNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    const auto& [varLb, varUb] = _variables[idx]->bounds();

    lb += _coeffs[idx] * varLb;
    ub += _coeffs[idx] * varUb;
  }

  return {lb, ub};
}
