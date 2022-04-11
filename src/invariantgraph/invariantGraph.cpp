#include "invariantgraph/invariantGraph.hpp"

#include <numeric>
#include <queue>
#include <unordered_set>

#include "invariants/linear.hpp"
#include "utils/fznAst.hpp"

static Int totalViolationsUpperBound(Engine& engine,
                                     const std::vector<VarId>& violations) {
  return std::accumulate(
      violations.begin(), violations.end(), 0,
      [&](auto sum, const auto& var) { return sum + engine.upperBound(var); });
}

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

invariantgraph::InvariantGraphApplyResult invariantgraph::InvariantGraph::apply(
    Engine& engine) {
  engine.open();

  std::unordered_map<VariableNode*, VarId> variableIds;
  std::vector<VarId> violations;

  std::queue<invariantgraph::VariableDefiningNode*> unAppliedNodes;
  std::unordered_set<invariantgraph::VariableDefiningNode*> seenNodes;
  for (const auto& implicitConstraint : _implicitConstraints) {
    unAppliedNodes.push(implicitConstraint);
  }

  while (!unAppliedNodes.empty()) {
    auto node = unAppliedNodes.front();
    seenNodes.insert(node);

    node->registerWithEngine(engine, variableIds);

    if (auto violationNode = node->violation()) {
      violations.push_back(variableIds.at(violationNode));
    }

    for (const auto& definedVariable : node->definedVariables()) {
      for (const auto definingNode : definedVariable->inputFor()) {
        if (seenNodes.find(definingNode) == seenNodes.end()) {
          unAppliedNodes.push(definingNode);
          seenNodes.insert(definingNode);
        }
      }
    }

    unAppliedNodes.pop();
  }

  VarId totalViolations =
      engine.makeIntVar(0, 0, totalViolationsUpperBound(engine, violations));
  engine.makeInvariant<Linear>(violations, totalViolations);

  // If the model has no variable to optimise, use a dummy variable.
  VarId objectiveVarId = _objectiveVariable == nullptr
                             ? engine.makeIntVar(0, 0, 0)
                             : variableIds.at(_objectiveVariable);

  engine.close();

  return {createVariableMap(variableIds), _implicitConstraints, totalViolations,
          objectiveVarId};
}
