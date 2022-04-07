#include "invariantgraph/structure.hpp"

void invariantgraph::ImplicitConstraintNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  std::vector<search::SearchVariable> varIds;
  varIds.reserve(definedVariables().size());

  for (const auto& node : definedVariables()) {
    const auto& [lb, ub] = node->bounds();
    auto varId = engine.makeIntVar(lb, lb, ub);

    variableMap.emplace(node, varId);
    varIds.emplace_back(varId, node->domain());
  }

  _neighbourhood = createNeighbourhood(engine, varIds);
  assert(_neighbourhood);
}
