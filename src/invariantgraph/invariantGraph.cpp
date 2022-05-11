#include "invariantgraph/invariantGraph.hpp"

#include <deque>
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

void invariantgraph::InvariantGraph::breakCycles() {}

invariantgraph::InvariantGraphApplyResult invariantgraph::InvariantGraph::apply(
    Engine& engine) {
  engine.open();

  std::unordered_map<VariableNode*, VarId> variableMap;
  std::unordered_set<invariantgraph::VariableDefiningNode*> visitedNodes;
  std::queue<invariantgraph::VariableDefiningNode*> unregisteredNodes;

  for (auto& node : _valueNodes) {
    assert(node->isConstant());

    auto val = node->bounds().first;
    variableMap.emplace(node, engine.makeIntVar(val, val, val));
  }

  for (auto* const implicitConstraint : _implicitConstraints) {
    visitedNodes.emplace(implicitConstraint);
    unregisteredNodes.emplace(implicitConstraint);
  }

  std::vector<VarId> violations;
  std::unordered_set<VariableNode*> definedVariables;

  while (!unregisteredNodes.empty()) {
    auto* const node = unregisteredNodes.front();
    unregisteredNodes.pop();
    assert(visitedNodes.contains(node));
    // If the node only defines a single variable, then it is a view:
    assert((node->dynamicInputs().size() == 0 &&
            node->staticInputs().size() != 1) ||
           variableMap.contains(node->staticInputs().front()));
#ifndef NDEBUG
    for (auto* const definedVariable : node->definedVariables()) {
      assert(!variableMap.contains(definedVariable));
    }
#endif
    node->createDefinedVariables(engine, variableMap);
    for (auto* const definedVariable : node->definedVariables()) {
      definedVariables.emplace(definedVariable);
      assert(variableMap.contains(definedVariable));
      for (auto* const variableNode : definedVariable->inputFor()) {
        if (!visitedNodes.contains(variableNode)) {
          visitedNodes.emplace(variableNode);
          unregisteredNodes.emplace(variableNode);
        }
      }
    }
  }

  for (auto* const node : visitedNodes) {
#ifndef NDEBUG
    for (auto* const definedVariable : node->definedVariables()) {
      variableMap.contains(definedVariable);
    }
#endif
    node->registerWithEngine(engine, variableMap);
    if (!node->isReified()) {
      if (auto* const violationNode = node->violation()) {
        violations.push_back(variableMap.at(violationNode));
      }
    }
  }

  engine.computeBounds();

  for (auto* node : definedVariables) {
    node->postDomainConstraint(engine, variableMap,
                               node->constrainedDomain(engine, variableMap));
  }

  const VarId totalViolations = engine.makeIntVar(0, 0, 0);
  engine.makeInvariant<Linear>(violations, totalViolations);

  // If the model has no variable to optimise, use a dummy variable.
  VarId objectiveVarId = _objectiveVariable == nullptr
                             ? engine.makeIntVar(0, 0, 0)
                             : variableMap.at(_objectiveVariable);

  engine.close();

  return {createVariableMap(variableMap), _implicitConstraints, totalViolations,
          objectiveVarId};
}
