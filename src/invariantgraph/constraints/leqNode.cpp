#include "invariantgraph/constraints/leqNode.hpp"

#include <algorithm>
#include <limits>

#include "invariantgraph/parseHelper.hpp"
#include "constraints/lessEqual.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::LeqNode>
invariantgraph::LeqNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_lin_le");
  assert(constraint->arguments().size() == 3);

  VALUE_VECTOR_ARG(coeffs, constraint->arguments()[0]);
  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(variables, constraint->arguments()[1],
                             variableMap);
  VALUE_ARG(bound, constraint->arguments()[2]);

  return std::make_unique<LeqNode>(coeffs, variables, bound);
}

VarId invariantgraph::LeqNode::registerWithEngine(
    Engine &engine, std::function<VarId(VariableNode *)> variableMapper) const {
  auto [sumLb, sumUb] = domainBounds();
  auto sumVar = engine.makeIntVar(0, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMapper(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  Int violationUb = std::max<Int>(0, std::numeric_limits<Int>::max() - _bound);
  auto violation = engine.makeIntVar(0, 0, violationUb);
  auto bound = engine.makeIntVar(_bound, _bound, _bound);
  engine.makeConstraint<LessEqual>(violation, sumVar, bound);

  return violation;
}

std::pair<Int, Int> invariantgraph::LeqNode::domainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    lb += _coeffs[idx] * _variables[idx]->variable()->domain()->lowerBound();
    ub += _coeffs[idx] * _variables[idx]->variable()->domain()->upperBound();
  }

  return std::make_pair(lb, ub);
}
