#include "../parseHelper.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"
#include "invariants/elementVar.hpp"

std::unique_ptr<invariantgraph::ArrayVarIntElementNode>
invariantgraph::ArrayVarIntElementNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "array_var_int_element");
  assert(constraint->arguments().size() == 3);

  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(as, constraint->arguments()[1],
                                    variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(c, constraint->arguments()[2], variableMap);

  return std::make_unique<invariantgraph::ArrayVarIntElementNode>(as, b, c);
}

void invariantgraph::ArrayVarIntElementNode::registerWithEngine(
    Engine &engine, std::map<VariableNode *, VarId> &variableMap) {
  std::vector<VarId> as;
  std::transform(_as.begin(), _as.end(), std::back_inserter(as),
                 [&](auto var) { return variableMap.at(var); });

  auto outputId =
      registerDefinedVariable(engine, variableMap, definedVariables()[0]);

  engine.makeInvariant<ElementVar>(variableMap.at(_b), as, outputId);
}
