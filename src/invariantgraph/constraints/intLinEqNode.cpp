#include "invariantgraph/constraints/intLinEqNode.hpp"

#include "constraints/equal.hpp"
#include "invariants/linear.hpp"
#include "invariantgraph/parseHelper.hpp"

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

VarId invariantgraph::IntLinEqNode::registerWithEngine(
    Engine &engine, std::function<VarId(VariableNode *)> variableMapper) const {
  auto [sumLb, sumUb] = getDomainBounds();
  auto sumVar = engine.makeIntVar(0, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMapper(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  Int violationUb = std::max<Int>(0, std::numeric_limits<Int>::max() - _c);
  auto violation = engine.makeIntVar(0, 0, violationUb);
  auto c = engine.makeIntVar(_c, _c, _c);
  engine.makeConstraint<Equal>(violation, sumVar, c);

  return violation;
}

std::pair<Int, Int>
invariantgraph::IntLinEqNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    lb += _coeffs[idx] * _variables[idx]->variable()->domain()->lowerBound();
    ub += _coeffs[idx] * _variables[idx]->variable()->domain()->upperBound();
  }

  return std::make_pair(lb, ub);
}
