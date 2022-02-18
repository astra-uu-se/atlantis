#include "invariantgraph/constraints/lessThanEq.hpp"

#include <algorithm>
#include <limits>

#include "constraints/lessEqual.hpp"
#include "invariants/linear.hpp"

std::unique_ptr<invariantgraph::LessThanEqNode>
invariantgraph::LessThanEqNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_lin_le");
  assert(constraint->arguments().size() == 3);

  auto coeffLiterals =
      std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(
          constraint->arguments()[0]);
  auto varLiterals = std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(
      constraint->arguments()[1]);
  auto boundLiteral =
      std::get<std::shared_ptr<fznparser::Literal>>(constraint->arguments()[2]);

  std::vector<Int> coeffs;
  coeffs.reserve(coeffLiterals.size());
  std::transform(
      coeffLiterals.begin(), coeffLiterals.end(), std::back_inserter(coeffs),
      [](const auto &coeff) {
        return std::dynamic_pointer_cast<fznparser::ValueLiteral>(coeff)
            ->value();
      });

  std::vector<invariantgraph::VariableNode *> variables;
  variables.reserve(varLiterals.size());
  std::transform(varLiterals.begin(), varLiterals.end(),
                 std::back_inserter(variables), [&](const auto &literal) {
                   auto var =
                       std::dynamic_pointer_cast<fznparser::Variable>(literal);

                   return variableMap(var);
                 });

  Int bound =
      std::static_pointer_cast<fznparser::ValueLiteral>(boundLiteral)->value();

  return std::make_unique<LessThanEqNode>(coeffs, variables, bound);
}

VarId invariantgraph::LessThanEqNode::registerWithEngine(
    Engine &engine, std::function<VarId(VariableNode *)> variableMapper) const {
  auto [sumLb, sumUb] = getDomainBounds();
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

std::pair<Int, Int> invariantgraph::LessThanEqNode::getDomainBounds() const {
  Int lb = 0;
  Int ub = 0;

  for (size_t idx = 0; idx < _coeffs.size(); idx++) {
    lb += _coeffs[idx] * _variables[idx]->variable()->domain()->lowerBound();
    ub += _coeffs[idx] * _variables[idx]->variable()->domain()->upperBound();
  }

  return std::make_pair(lb, ub);
}
