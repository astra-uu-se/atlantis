#include "invariantgraph/invariantGraph.hpp"

#include <deque>
#include <queue>
#include <stack>
#include <unordered_set>

#include "invariants/linear.hpp"
#include "utils/fznAst.hpp"

static invariantgraph::InvariantGraphApplyResult::VariableMap createVariableMap(
    const std::vector<invariantgraph::VariableNode*>& variableNodes) {
  invariantgraph::InvariantGraphApplyResult::VariableMap variableMap{};

  for (const auto& node : variableNodes) {
    if (node->variable()) {
      std::visit(
          [&](const auto& variable) {
            variableMap.emplace(node->varId(), variable.name);
          },
          *node->variable());
    }
  }
  return variableMap;
}

VarId invariantgraph::InvariantGraph::createViolations(Engine& engine) {
  std::vector<VarId> violations;
  for (const auto& definingNode : _variableDefiningNodes) {
    if (!definingNode->isReified() &&
        definingNode->violationVarId() != NULL_ID) {
      violations.emplace_back(definingNode->violationVarId());
    }
  }

  std::unordered_set<VariableNode*> searchVariables;
  for (auto* const implicitConstraint : _implicitConstraints) {
    for (auto* const searchNode : implicitConstraint->definedVariables()) {
      searchVariables.emplace(searchNode);
    }
  }
  for (auto& node : _variables) {
    if (!searchVariables.contains(node.get())) {
      const VarId violationId =
          node->postDomainConstraint(engine, node->constrainedDomain(engine));
      if (violationId != NULL_ID) {
        violations.emplace_back(violationId);
      }
    }
  }
  if (violations.empty()) {
    return NULL_ID;
  }
  if (violations.size() == 1) {
    return violations.front();
  }
  const VarId totalViolation = engine.makeIntVar(0, 0, 0);
  engine.makeInvariant<Linear>(violations, totalViolation);
  return totalViolation;
}

void invariantgraph::InvariantGraph::createVariables(Engine& engine) {
  std::unordered_set<invariantgraph::VariableDefiningNode*> visitedNodes;
  std::unordered_set<invariantgraph::VariableNode*> searchVariables;

  std::queue<invariantgraph::VariableDefiningNode*> unregisteredNodes;

  for (auto* const implicitConstraint : _implicitConstraints) {
    visitedNodes.emplace(implicitConstraint);
    unregisteredNodes.emplace(implicitConstraint);
    for (auto* const searchNode : implicitConstraint->definedVariables()) {
      searchVariables.emplace(searchNode);
    }
  }

  std::vector<VarId> violations;
  std::unordered_set<VariableNode*> definedVariables;

  while (!unregisteredNodes.empty()) {
    auto* const node = unregisteredNodes.front();
    unregisteredNodes.pop();
    assert(visitedNodes.contains(node));
    // If the node only defines a single variable, then it is a view:
    assert(!node->dynamicInputs().empty() || node->staticInputs().size() != 1 ||
           node->staticInputs().front()->varId() != NULL_ID);
#ifndef NDEBUG
    for (auto* const definedVariable : node->definedVariables()) {
      assert(definedVariable->varId() == NULL_ID);
    }
#endif
    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      definedVariables.emplace(definedVariable);
      assert(definedVariable->varId() != NULL_ID);
      for (auto* const variableNode : definedVariable->inputFor()) {
        if (!visitedNodes.contains(variableNode)) {
          visitedNodes.emplace(variableNode);
          unregisteredNodes.emplace(variableNode);
        }
      }
    }
  }
  assert(visitedNodes.size() == _variableDefiningNodes.size());
}

void invariantgraph::InvariantGraph::createInvariants(Engine& engine) {
  for (const auto& node : _variableDefiningNodes) {
#ifndef NDEBUG
    for (auto* const definedVariable : node->definedVariables()) {
      assert(definedVariable->varId() != NULL_ID);
    }
#endif
    node->registerWithEngine(engine);
  }
}

invariantgraph::InvariantGraphApplyResult invariantgraph::InvariantGraph::apply(
    Engine& engine) {
  engine.open();
  createVariables(engine);
  engine.computeBounds();
  createInvariants(engine);
  const VarId totalViolation = createViolations(engine);
  // If the model has no variable to optimise, use a dummy variable.
  VarId objectiveVarId = _objectiveVariable != nullptr
                             ? _objectiveVariable->varId()
                             : engine.makeIntVar(0, 0, 0);
  engine.close();

  std::vector<VariableNode*> variables(
      _variables.size() + (_objectiveVariable != nullptr ? 1 : 0), nullptr);
  for (size_t i = 0; i < _variables.size(); ++i) {
    variables[i] = _variables[i].get();
  }
  if (_objectiveVariable != nullptr) {
    variables.back() = _objectiveVariable;
  }

  return {createVariableMap(variables), _implicitConstraints, totalViolation,
          objectiveVarId};
}
