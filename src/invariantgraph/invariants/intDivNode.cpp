#include "invariantgraph/invariants/intDivNode.hpp"

#include "invariantgraph/parseHelper.hpp"
#include "invariants/intDiv.hpp"

std::unique_ptr<invariantgraph::IntDivNode>
invariantgraph::IntDivNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "int_div");
  assert(constraint->annotations().has<fznparser::DefinesVarAnnotation>());
  assert(constraint->arguments().size() == 3);

  auto definedVar = constraint->annotations()
                        .get<fznparser::DefinesVarAnnotation>()
                        ->defines()
                        .lock();

  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(output, constraint->arguments()[2], variableMap);

  assert(definedVar == output->variable());

  return std::make_unique<invariantgraph::IntDivNode>(a, b, output);
}

void invariantgraph::IntDivNode::registerWithEngine(
    Engine& engine, std::function<VarId(VariableNode*)> variableMapper) const {
  engine.makeInvariant<::IntDiv>(variableMapper(_a), variableMapper(_b), variableMapper(output()));
}
