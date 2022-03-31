#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "search/neighbourhoods/allDifferentNeighbourhood.hpp"

std::unique_ptr<invariantgraph::AllDifferentImplicitNode>
invariantgraph::AllDifferentImplicitNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<invariantgraph::VariableNode*(
        std::shared_ptr<fznparser::Variable>)>& variableMap) {
  assert(constraint->name() == "alldifferent");
  assert(constraint->arguments().size() == 1);

  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(variables, constraint->arguments()[0],
                                    variableMap);

  // For now, this only works when all the variables have the same domain.
  const auto& domain = variables.front()->domain();
  for (const auto& variable : variables) {
    if (variable->domain() != domain) {
      return nullptr;
    }
  }

  return std::make_unique<AllDifferentImplicitNode>(variables);
}

invariantgraph::AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {
  assert(definedVariables().size() > 1);

  const auto& domain = definedVariables().front()->domain();
  for (const auto& variable : definedVariables()) {
    assert(variable->domain() == domain);
  }
}

search::neighbourhoods::Neighbourhood*
invariantgraph::AllDifferentImplicitNode::createNeighbourhood(
    Engine& engine, const std::vector<VarId>& variables) {
  auto domainValues = definedVariables().front()->domain().values();

  return new search::neighbourhoods::AllDifferentNeighbourhood(
      variables, domainValues, engine);
}
