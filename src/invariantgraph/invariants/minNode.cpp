#include "invariantgraph/invariants/minNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/minSparse.hpp"

std::unique_ptr<invariantgraph::MinNode>
invariantgraph::MinNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "array_int_minimum");
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());
  assert(constraint->arguments().size() == 2);

  auto definedVar = constraint->annotations()
                        .get<fznparser::DefinesVarAnnotation>()
                        ->defines()
                        .lock();

  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(inputs, constraint->arguments()[1], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(output, constraint->arguments()[0], variableMap);
  assert(definedVar == output->variable());

  return std::make_unique<invariantgraph::MinNode>(inputs, output);
}

void invariantgraph::MinNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::MinSparse>(variables, variableMapper(output()));
}
