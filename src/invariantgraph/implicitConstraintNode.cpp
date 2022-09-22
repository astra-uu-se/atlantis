#include "invariantgraph/implicitConstraintNode.hpp"

void invariantgraph::ImplicitConstraintNode::createDefinedVariables(
    Engine& engine) {
  for (const auto& node : definedVariables()) {
    if (node->varId(this) == NULL_ID) {
      const auto& [lb, ub] = node->bounds();
      assert(node->isSearchVariable());
      node->setVarId(engine.makeIntVar(lb, lb, ub));
    }
  }
}

void invariantgraph::ImplicitConstraintNode::registerWithEngine(
    Engine& engine) {
  std::vector<search::SearchVariable> varIds;
  varIds.reserve(definedVariables().size());

  for (const auto& node : definedVariables()) {
    assert(node->isSearchVariable());
    assert(node->inputVarId() != NULL_ID);
    varIds.emplace_back(node->inputVarId(), node->domain());
  }

  _neighbourhood = createNeighbourhood(engine, varIds);
  assert(_neighbourhood);
}