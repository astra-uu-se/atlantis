#include "invariantgraph/invariants/maxNode.hpp"

#include <algorithm>

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

  auto inputs = std::get<std::vector<std::shared_ptr<fznparser::Literal>>>(
      constraint->arguments()[1]);

  std::vector<VariableNode*> inputNodes;
  std::transform(inputs.begin(), inputs.end(), std::back_inserter(inputNodes),
                 [&variableMap](const auto& var) {
                   return variableMap(
                       std::dynamic_pointer_cast<fznparser::Variable>(var));
                 });

  auto output = std::get<std::shared_ptr<fznparser::Literal>>(
      constraint->arguments()[0]);
  assert(definedVar == output);

  auto outputNode =
      variableMap(std::dynamic_pointer_cast<fznparser::Variable>(output));

  return std::make_unique<invariantgraph::MaxNode>(inputNodes, outputNode);
}

void invariantgraph::MaxNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  std::vector<VarId> variables;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(variables), variableMapper);

  engine.makeInvariant<::MaxSparse>(variables, variableMapper(output()));
}
