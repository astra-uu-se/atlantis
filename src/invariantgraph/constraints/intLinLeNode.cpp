#include "invariantgraph/constraints/intLinLeNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "constraints/lessEqual.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::IntLinLeNode>
invariantgraph::IntLinLeNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_lin_le");
  assert(constraint->arguments().size() == 3);

  VALUE_VECTOR_ARG(coeffs, constraint->arguments()[0]);
  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(variables, constraint->arguments()[1],
                             variableMap);
  VALUE_ARG(bound, constraint->arguments()[2]);

  return std::make_unique<IntLinLeNode>(coeffs, variables, bound);
}

void invariantgraph::IntLinLeNode::registerWithEngine(
    Engine &engine, std::map<VariableNode*, VarId>& variableMap) {
  auto [sumLb, sumUb] = getDomainBounds();
  auto sumVar = engine.makeIntVar(0, sumLb, sumUb);

  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables),
                 [&](auto var) { return variableMap.at(var); });
  engine.makeInvariant<Linear>(_coeffs, variables, sumVar);

  auto violation = registerViolation(engine, variableMap);
  auto bound = engine.makeIntVar(_bound, _bound, _bound);
  engine.makeConstraint<LessEqual>(violation, sumVar, bound);
}

std::pair<Int, Int> invariantgraph::IntLinLeNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    lb += _coeffs[idx] * _variables[idx]->variable()->domain()->lowerBound();
    ub += _coeffs[idx] * _variables[idx]->variable()->domain()->upperBound();
  }

  return std::make_pair(lb, ub);
}
