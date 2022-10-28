#include "invariantgraph/invariantGraph.hpp"

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

void invariantgraph::InvariantGraph::splitMultiDefinedVariables() {
  for (size_t i = 0; i < _variables.size(); ++i) {
    if (_variables[i]->definingNodes().size() <= 1) {
      continue;
    }

    std::vector<VariableDefiningNode*> replacedDefiningNodes;
    std::vector<VariableNode*> splitNodes;
    replacedDefiningNodes.reserve(_variables[i]->definingNodes().size() - 1);
    splitNodes.reserve(_variables[i]->definingNodes().size());
    splitNodes.emplace_back(_variables[i].get());

    for (auto iter = ++_variables[i]->definingNodes().begin();
         iter != _variables[i]->definingNodes().end(); ++iter) {
      replacedDefiningNodes.emplace_back(*iter);
    }

    for (auto* const definingNode : replacedDefiningNodes) {
      VariableNode* newNode = splitNodes.emplace_back(
          _variables
              .emplace_back(std::make_unique<VariableNode>(
                  _variables[i]->domain(), _variables[i]->isIntVar()))
              .get());
      definingNode->replaceDefinedVariable(_variables[i].get(), newNode);
    }
    if (_variables[i]->isIntVar()) {
      if (splitNodes.size() == 2) {
        _variableDefiningNodes.emplace_back(
            std::make_unique<IntEqNode>(splitNodes.front(), splitNodes.back()));
      } else {
        _variableDefiningNodes.emplace_back(
            std::make_unique<AllEqualNode>(splitNodes));
      }
    }
  }
}

std::pair<invariantgraph::VariableNode*, invariantgraph::VariableDefiningNode*>
invariantgraph::InvariantGraph::findPivotInCycle(
    const std::vector<VariableNode*>& cycle) {
  assert(cycle.size() >= 1);
  VariableNode* pivot = nullptr;
  VariableDefiningNode* listeningInvariant = nullptr;
  size_t maxDomainSize = 0;
  for (size_t i = 0; i < cycle.size(); ++i) {
    if (cycle[i]->domain().size() > maxDomainSize) {
      maxDomainSize = cycle[i]->domain().size();
      pivot = cycle[i];
      listeningInvariant = cycle[(i + 1) % cycle.size()]->definedBy();
      assert(std::any_of(
          listeningInvariant->staticInputs().begin(),
          listeningInvariant->staticInputs().end(),
          [&](VariableNode* const input) { return input == pivot; }));
    }
  }
  assert(pivot != nullptr);
  assert(maxDomainSize > 0);
  return std::pair<VariableNode*, VariableDefiningNode*>{pivot,
                                                         listeningInvariant};
}

std::vector<invariantgraph::VariableNode*>
invariantgraph::InvariantGraph::findCycle(
    const std::unordered_map<VariableNode*, VariableNode*>& childOf,
    VariableNode* const node, VariableNode* const parent) {
  std::vector<VariableNode*> cycle;
  size_t i = 0;
  for (VariableNode* cur = parent; cur != node; cur = childOf.at(cur)) {
    if (i++ == childOf.size()) {
      break;
    }
    cycle.push_back(cur);
  }
  assert(cycle.size() >= 1);
  assert(cycle.front() == parent);
  assert(cycle.back() != node);
  cycle.emplace_back(node);
  assert(cycle.back() == node);
  return cycle;
}

invariantgraph::VariableNode* invariantgraph::InvariantGraph::breakCycle(
    const std::vector<VariableNode*>& cycle) {
  auto [pivot, listeningInvariant] = findPivotInCycle(cycle);

  VariableNode* newInputNode = _variables
                                   .emplace_back(std::make_unique<VariableNode>(
                                       pivot->domain(), pivot->isIntVar()))
                                   .get();

  listeningInvariant->replaceStaticInput(pivot, newInputNode);
  _variableDefiningNodes.emplace_back(
      std::make_unique<IntEqNode>(pivot, newInputNode));
  assert(std::any_of(_implicitConstraints.begin(), _implicitConstraints.end(),
                     [&](ImplicitConstraintNode* const implicitConstraint) {
                       return dynamic_cast<InvariantGraphRoot*>(
                                  implicitConstraint) != nullptr;
                     }));
  for (auto* const implicitConstraint : _implicitConstraints) {
    assert(listeningInvariant != implicitConstraint);
    if (InvariantGraphRoot* root =
            dynamic_cast<InvariantGraphRoot*>(implicitConstraint);
        root != nullptr) {
      root->addSearchVariable(newInputNode);
    }
  }
  return newInputNode;
}

static std::unordered_set<invariantgraph::VariableNode*>
dynamicVariableNodePenumbra(
    invariantgraph::VariableNode* const node,
    const std::unordered_set<invariantgraph::VariableNode*>& visitedGlobal) {
  std::unordered_set<invariantgraph::VariableNode*> visitedLocal;
  std::unordered_set<invariantgraph::VariableNode*> visitedDynamic;
  std::queue<invariantgraph::VariableNode*> q;
  q.push(node);
  visitedLocal.emplace(node);
  while (!q.empty()) {
    auto* const cur = q.front();
    q.pop();
    // Add unvisited dynamic neighbours to set of visited dynamic variable
    // nodes:
    for (auto* const dynamicInv : cur->dynamicInputFor()) {
      for (auto* const dynamicNode : dynamicInv->definedVariables()) {
        if (!visitedGlobal.contains(dynamicNode) &&
            !visitedDynamic.contains(dynamicNode)) {
          visitedDynamic.emplace(dynamicNode);
        }
      }
    }
    // add unvisited static neighbours to queue:
    for (auto* const staticInv : cur->staticInputFor()) {
      for (auto* const staticNode : staticInv->definedVariables()) {
        if (!visitedLocal.contains(staticNode)) {
          visitedLocal.emplace(staticNode);
          q.push(staticNode);
        }
      }
    }
  }
  return visitedDynamic;
}

static invariantgraph::VariableNode* findCycleUtil(
    invariantgraph::VariableNode* const node,
    const std::unordered_set<invariantgraph::VariableNode*>& visitedGlobal,
    std::unordered_set<invariantgraph::VariableNode*>& visitedLocal,
    std::unordered_map<invariantgraph::VariableNode*,
                       invariantgraph::VariableNode*>& path) {
  if (visitedGlobal.contains(node)) {
    return nullptr;
  }
  visitedLocal.emplace(node);
  for (auto* const listeningNode : node->staticInputFor()) {
    for (auto* const definedVar : listeningNode->definedVariables()) {
      if (!visitedGlobal.contains(definedVar) &&
          !visitedLocal.contains(definedVar)) {
        path.emplace(node, definedVar);
        auto* const cycleRoot =
            findCycleUtil(definedVar, visitedGlobal, visitedLocal, path);
        if (cycleRoot != nullptr) {
          return cycleRoot;
        }
      } else if (path.contains(definedVar)) {
        return definedVar;
      }
    }
  }
  path.erase(node);
  return nullptr;
}

std::vector<invariantgraph::VariableNode*>
invariantgraph::InvariantGraph::breakCycles(
    invariantgraph::VariableNode* node,
    std::unordered_set<invariantgraph::VariableNode*>& visitedGlobal) {
  std::unordered_map<invariantgraph::VariableNode*, VariableNode*> path;
  std::unordered_set<invariantgraph::VariableNode*> visitedLocal;
  invariantgraph::VariableNode* cycleRoot;
  std::vector<VariableNode*> newSearchNodes;
  do {
    path.clear();
    visitedLocal.clear();
    cycleRoot = findCycleUtil(node, visitedGlobal, visitedLocal, path);
    if (cycleRoot != nullptr) {
      std::vector<invariantgraph::VariableNode*> cycle;
      invariantgraph::VariableNode* cur = cycleRoot;
      for (; path.contains(cur); cur = path.at(cur)) {
        cycle.push_back(cur);
      }
      assert(std::all_of(
          cycle.begin(), cycle.end(),
          [&](invariantgraph::VariableNode* n) { return n != cur; }));
      cycle.push_back(cur);
      newSearchNodes.emplace_back(breakCycle(cycle));
      assert(newSearchNodes.back() != nullptr);
    }
  } while (cycleRoot != nullptr);

  // add the visited nodes to the global set of visited nodes
  for (auto* const visitedNode : visitedLocal) {
    assert(!visitedGlobal.contains(visitedNode));
    visitedGlobal.emplace(visitedNode);
  }
  return newSearchNodes;
}

void invariantgraph::InvariantGraph::breakCycles() {
  std::unordered_set<VariableNode*> visitedGlobal;
  std::queue<VariableNode*> searchNodes;

  for (auto* const implicitConstraint : _implicitConstraints) {
    for (auto* const searchNode : implicitConstraint->definedVariables()) {
      searchNodes.emplace(searchNode);
    }
  }

  while (!searchNodes.empty()) {
    std::vector<VariableNode*> visitedSearchNodes;
    while (!searchNodes.empty()) {
      VariableNode* searchNode = searchNodes.front();
      searchNodes.pop();
      visitedSearchNodes.emplace_back(searchNode);
      for (auto* newSearchNode : breakCycles(searchNode, visitedGlobal)) {
        searchNodes.emplace(newSearchNode);
      }
    }
    // If some part of the invariant graph is unreachable if not traversing
    // dynamic inputs of dynamic invariants, then add those dynamic inputs to
    // the set of search nodes:
    std::unordered_set<VariableNode*> dynamicSearchNodes;
    for (auto* searchNode : visitedSearchNodes) {
      for (auto* const dynamicNode :
           dynamicVariableNodePenumbra(searchNode, visitedGlobal)) {
        assert(!visitedGlobal.contains(dynamicNode));
        if (!dynamicSearchNodes.contains(dynamicNode)) {
          searchNodes.emplace(dynamicNode);
          dynamicSearchNodes.emplace(dynamicNode);
        }
      }
    }
  }
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
    assert(std::all_of(node->definedVariables().begin(),
                       node->definedVariables().end(),
                       [&](VariableNode* const definedVariable) {
                         return definedVariable->varId() == NULL_ID;
                       }));

    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      assert(definedVariable->definedBy() == node);
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

  for (auto* const valueNode : _valueNodes) {
    if (valueNode->varId() == NULL_ID) {
      auto constant = valueNode->constantValue();
      assert(constant);
      auto varId = engine.makeIntVar(*constant, *constant, *constant);
      valueNode->setVarId(varId);
    }
  }

  assert(std::all_of(
      _variableDefiningNodes.begin(), _variableDefiningNodes.end(),
      [&](const auto& node) { return visitedNodes.contains(node.get()); }));
}

void invariantgraph::InvariantGraph::createInvariants(Engine& engine) {
  for (const auto& node : _variableDefiningNodes) {
    assert(std::all_of(node->definedVariables().begin(),
                       node->definedVariables().end(),
                       [&](auto* const definedVariable) {
                         return definedVariable->varId() != NULL_ID;
                       }));
    node->registerWithEngine(engine);
  }
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
  engine.makeInvariant<Linear>(engine, totalViolation, violations);
  return totalViolation;
}

invariantgraph::InvariantGraphApplyResult invariantgraph::InvariantGraph::apply(
    Engine& engine) {
  splitMultiDefinedVariables();
  breakCycles();
  engine.open();
  createVariables(engine);
  createInvariants(engine);
  engine.computeBounds();
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
