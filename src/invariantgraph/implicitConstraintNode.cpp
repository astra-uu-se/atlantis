#include "invariantgraph/implicitConstraintNode.hpp"

void invariantgraph::ImplicitConstraintNode::createDefinedVariables(
    Engine& engine) {
  for (const auto& node : definedVariables()) {
    if (node->varId() == NULL_ID) {
      const auto& [lb, ub] = node->bounds();
      node->setVarId(engine.makeIntVar(lb, lb, ub));
    }
  }
}

void invariantgraph::ImplicitConstraintNode::registerWithEngine(
    Engine& engine) {
  std::vector<search::SearchVariable> varIds;
  varIds.reserve(definedVariables().size());

  for (const auto& node : definedVariables()) {
    assert(node->varId() != NULL_ID);
    varIds.emplace_back(node->varId(), node->domain());
  }

  _neighbourhood = createNeighbourhood(engine, varIds);
  assert(_neighbourhood);
}