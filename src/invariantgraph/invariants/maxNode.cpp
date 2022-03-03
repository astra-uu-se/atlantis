#include "invariantgraph/invariants/maxNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "invariants/maxSparse.hpp"

std::unique_ptr<invariantgraph::MaxNode>
invariantgraph::MaxNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "array_int_maximum");
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());

  auto definedVar = constraint->annotations()
                        .get<fznparser::DefinesVarAnnotation>()
                        ->defines()
                        .lock();

  assert(constraint->arguments().size() == 2);

  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(inputs, constraint->arguments()[1], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(output, constraint->arguments()[0], variableMap);
  assert(definedVar == output->variable());

  return std::make_unique<invariantgraph::MaxNode>(inputs, output);
}

void invariantgraph::MaxNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::MaxSparse>(variables, variableMapper(output()));
}
