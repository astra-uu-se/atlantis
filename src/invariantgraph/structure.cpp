#include "invariantgraph/structure.hpp"

void invariantgraph::ImplicitConstraintNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  std::vector<VarId> varIds;
  varIds.reserve(definedVariables().size());

  for (const auto& node : definedVariables()) {
    const auto& [lb, ub] = node->domain();
    auto varId = engine.makeIntVar(lb, lb, ub);

    variableMap.emplace(node, varId);
    varIds.push_back(varId);
  }

  _neighbourhood = createNeighbourhood(engine, varIds);
  assert(_neighbourhood);
}
