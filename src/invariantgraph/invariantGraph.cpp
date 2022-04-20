#include "invariantgraph/invariantGraph.hpp"

#include <queue>
#include <stack>
#include <unordered_set>

#include "invariants/linear.hpp"
#include "utils/fznAst.hpp"

static invariantgraph::InvariantGraphApplyResult::VariableMap createVariableMap(
    const std::unordered_map<invariantgraph::VariableNode*, VarId>&
        variableIds) {
  invariantgraph::InvariantGraphApplyResult::VariableMap variableMap{};

  for (const auto& [node, varId] : variableIds) {
    if (node->variable()) {
      std::visit(
          [&, varId = varId](const auto& variable) {
            variableMap.emplace(varId, variable.name);
          },
          *node->variable());
    }
  }
  return variableMap;
}

static void topologicallySortedNodeUtil(
    invariantgraph::VariableDefiningNode* node,
    std::unordered_set<invariantgraph::VariableDefiningNode*>& visited,
    std::stack<invariantgraph::VariableDefiningNode*>& stack) {
  visited.emplace(node);

  for (const auto& variableNode : node->definedVariables()) {
    for (const auto& definingNode : variableNode->inputFor()) {
      if (!visited.contains(definingNode)) {
        topologicallySortedNodeUtil(definingNode, visited, stack);
      }
    }
  }

  stack.push(node);
}

static std::stack<invariantgraph::VariableDefiningNode*>
topologicallySortedNodes(
    const std::vector<invariantgraph::ImplicitConstraintNode*>&
        implicitConstraints) {
  std::unordered_set<invariantgraph::VariableDefiningNode*> visited;
  std::stack<invariantgraph::VariableDefiningNode*> stack;

  for (const auto& implicitConstraint : implicitConstraints) {
    topologicallySortedNodeUtil(implicitConstraint, visited, stack);
  }

  return stack;
}

invariantgraph::InvariantGraphApplyResult invariantgraph::InvariantGraph::apply(
    Engine& engine) {
  engine.open();

  std::unordered_map<VariableNode*, VarId> variableIds;
  std::vector<VarId> violations;

  auto unAppliedNodes = topologicallySortedNodes(_implicitConstraints);

  while (!unAppliedNodes.empty()) {
    auto node = unAppliedNodes.top();

    node->registerWithEngine(engine, variableIds);

    if (auto violationNode = node->violation()) {
      violations.push_back(variableIds.at(violationNode));
    }

    unAppliedNodes.pop();
  }

  VarId totalViolations = engine.makeIntVar(0, 0, 0);
  engine.makeInvariant<Linear>(violations, totalViolations);

  // If the model has no variable to optimise, use a dummy variable.
  VarId objectiveVarId = _objectiveVariable == nullptr
                             ? engine.makeIntVar(0, 0, 0)
                             : variableIds.at(_objectiveVariable);

  engine.close();

  return {createVariableMap(variableIds), _implicitConstraints, totalViolations,
          objectiveVarId};
}
