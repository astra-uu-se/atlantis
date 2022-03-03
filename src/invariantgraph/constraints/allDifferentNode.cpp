#include "invariantgraph/constraints/allDifferentNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "constraints/allDifferent.hpp"

std::unique_ptr<invariantgraph::AllDifferentNode>
invariantgraph::AllDifferentNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "alldifferent");
  assert(constraint->arguments().size() == 1);

  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(variables, constraint->arguments()[0], variableMap);

  return std::make_unique<AllDifferentNode>(variables);
}

VarId invariantgraph::AllDifferentNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  VarId violation = engine.makeIntVar(0, 0, _variables.size());

  std::vector<VarId> engineVariables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(engineVariables), variableMapper);

  engine.makeConstraint<AllDifferent>(violation, engineVariables);

  return violation;
}