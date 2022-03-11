#include "invariantgraph/constraints/allDifferentNode.hpp"

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

void invariantgraph::AllDifferentNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  VarId violation = registerViolation(engine, variableMap);

  std::vector<VarId> engineVariables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(engineVariables), [&](const auto& var) {
                   return variableMap.at(var);
                 });

  engine.makeConstraint<AllDifferent>(violation, engineVariables);
}