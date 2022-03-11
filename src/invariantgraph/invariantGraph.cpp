#include "invariantgraph/invariantGraph.hpp"

#include <numeric>
#include <queue>

#include "invariants/linear.hpp"

static Int totalViolationsUpperBound(Engine& engine,
                                     const std::vector<VarId>& violations) {
  return std::accumulate(violations.begin(), violations.end(), 0,
                         [&](auto sum, const auto& var) {
                           return sum + engine.getUpperBound(var);
                         });
}

void invariantgraph::InvariantGraph::apply(Engine& engine) {
  engine.open();

  std::map<VariableNode*, VarId> variableIds;
  std::vector<VarId> violations;

  std::queue<invariantgraph::VariableDefiningNode*> unAppliedNodes;
  for (const auto& implicitConstraint : _implicitConstraints) {
    unAppliedNodes.push(implicitConstraint);
  }

  while (!unAppliedNodes.empty()) {
    auto node = unAppliedNodes.front();

    node->registerWithEngine(engine, variableIds);

    if (auto violationNode = node->violation()) {
      violations.push_back(variableIds.at(violationNode));
    }

    for (const auto& definedVariable : node->definedVariables()) {
      for (const auto& definingNode : definedVariable->inputFor()) {
        unAppliedNodes.push(definingNode);
      }
    }

    unAppliedNodes.pop();
  }

  VarId totalViolations =
      engine.makeIntVar(0, 0, totalViolationsUpperBound(engine, violations));
  engine.makeInvariant<Linear>(violations, totalViolations);

  engine.close();
}
