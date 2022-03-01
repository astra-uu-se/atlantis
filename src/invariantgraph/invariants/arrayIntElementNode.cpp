#include "invariantgraph/invariants/arrayIntElementNode.hpp"

#include "invariantgraph/parseHelper.hpp"
#include "invariants/elementConst.hpp"

std::unique_ptr<invariantgraph::ArrayIntElementNode>
invariantgraph::ArrayIntElementNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "array_int_element");
  assert(constraint->arguments().size() == 3);

  VALUE_VECTOR_ARG(as, constraint->arguments()[1]);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(c, constraint->arguments()[2], variableMap);

  return std::make_unique<invariantgraph::ArrayIntElementNode>(as, b, c);
}

void invariantgraph::ArrayIntElementNode::registerWithEngine(
    Engine &engine, std::function<VarId(VariableNode *)> variableMapper) const {
  engine.makeInvariant<ElementConst>(variableMapper(_b), _as,
                                     variableMapper(output()));
}
