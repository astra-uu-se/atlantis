#include "invariantgraph/invariantGraphRoot.hpp"

void invariantgraph::ImplicitConstraintNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  for (const auto& node : definedVariables()) {
    const auto& [lb, ub] = node->domain();
    variableMap.emplace(node, engine.makeIntVar(lb, lb, ub));
  }
}
