#include "invariantgraph/constraints/intLinEqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::IntLinEqNode>
invariantgraph::IntLinEqNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_lin_eq");
  assert(constraint->arguments().size() == 3);

  VALUE_VECTOR_ARG(coeffs, constraint->arguments()[0]);
  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(variables, constraint->arguments()[1],
                                    variableMap);
  VALUE_ARG(bound, constraint->arguments()[2]);

  return std::make_unique<IntLinEqNode>(coeffs, variables, bound);
}

void invariantgraph::IntLinEqNode::registerWithEngine(
    Engine &engine, std::map<VariableNode *, VarId> &variableMap) {
  auto [sumLb, sumUb] = getDomainBounds();
  auto sumVar = engine.makeIntVar(0, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violationVar = registerViolation(engine, variableMap);
  auto c = engine.makeIntVar(_c, _c, _c);
  engine.makeConstraint<Equal>(violationVar, sumVar, c);
}

invariantgraph::VariableNode::Domain
invariantgraph::IntLinEqNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    lb += _coeffs[idx] * _variables[idx]->variable()->domain()->lowerBound();
    ub += _coeffs[idx] * _variables[idx]->variable()->domain()->upperBound();
  }

  return {lb, ub};
}
