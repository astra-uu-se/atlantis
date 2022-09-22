#include "invariantgraph/invariantGraph.hpp"

static invariantgraph::InvariantGraphApplyResult::VariableMap createVariableMap(
    const std::vector<invariantgraph::VariableNode*>& variableNodes) {
  invariantgraph::InvariantGraphApplyResult::VariableMap variableMap{};

  for (const auto& node : variableNodes) {
    if (node->variable()) {
      std::visit(
          [&](const auto& variable) {
            variableMap.emplace(node->inputVarId(), variable.name);
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

    std::vector<InvariantNode*> replacedDefiningNodes;
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
        _invariantNodes.emplace_back(
            std::make_unique<IntEqNode>(splitNodes.front(), splitNodes.back()));
      } else {
        _invariantNodes.emplace_back(
            std::make_unique<AllEqualNode>(splitNodes));
      }
    }
  }
}

std::pair<invariantgraph::VariableNode*, invariantgraph::InvariantNode*>
invariantgraph::InvariantGraph::findPivotInCycle(
    const std::vector<std::pair<VariableNode*, InvariantNode*>>& cycle) {
  assert(cycle.size() >= 1);
  std::pair<VariableNode*, InvariantNode*> pivot{nullptr, nullptr};
  size_t minDomainSize = std::numeric_limits<size_t>::max();
  for (size_t i = 0; i < cycle.size(); ++i) {
    if (cycle[i].first->domain().size() < minDomainSize) {
      minDomainSize = cycle[i].first->domain().size();
      pivot = cycle[i];
    }
  }
  assert(pivot.first != nullptr && pivot.second != nullptr);
  assert(minDomainSize > 0);
  return pivot;
}

invariantgraph::VariableNode* invariantgraph::InvariantGraph::breakCycle(
    const std::vector<std::pair<VariableNode*, InvariantNode*>>& cycle) {
  auto [pivot, listeningInvariant] = findPivotInCycle(cycle);

  VariableNode* newInputNode = _variables
                                   .emplace_back(std::make_unique<VariableNode>(
                                       pivot->domain(), pivot->isIntVar()))
                                   .get();

  listeningInvariant->replaceStaticInput(pivot, newInputNode);
  _invariantNodes.emplace_back(
      std::make_unique<IntEqNode>(pivot, newInputNode));
  bool addedToSearchNodes = false;
  for (auto* const implicitConstraint : _implicitConstraints) {
    assert(listeningInvariant != implicitConstraint);
    if (InvariantGraphRoot* root =
            dynamic_cast<InvariantGraphRoot*>(implicitConstraint);
        root != nullptr) {
      addedToSearchNodes = true;
      root->addSearchVariable(newInputNode);
    }
  }
  assert(addedToSearchNodes);
  return newInputNode;
}

static std::unordered_map<invariantgraph::VariableNode*,
                          std::unordered_set<invariantgraph::InvariantNode*>>
dynamicVariableNodePenumbra(
    invariantgraph::VariableNode* const variableNode,
    const std::unordered_map<
        invariantgraph::VariableNode*,
        std::unordered_set<invariantgraph::InvariantNode*>>& visitedGlobal) {
  std::unordered_map<invariantgraph::VariableNode*,
                     std::unordered_set<invariantgraph::InvariantNode*>>
      visitedLocal;
  std::unordered_map<invariantgraph::VariableNode*,
                     std::unordered_set<invariantgraph::InvariantNode*>>
      visitedDynamic;
  std::queue<invariantgraph::VariableNode*> q;
  q.emplace(variableNode);
  visitedLocal.emplace(
      variableNode,
      std::unordered_set<invariantgraph::InvariantNode*>{nullptr});

  while (!q.empty()) {
    auto* const curVar = q.front();
    q.pop();
    // Add unvisited dynamic neighbours to set of visited dynamic variable
    // nodes:
    for (auto* const dynamicInv : curVar->dynamicInputFor()) {
      for (auto* const dynamicVar : dynamicInv->definedVariables()) {
        if (!visitedGlobal.contains(dynamicVar) &&
            !visitedGlobal.at(dynamicVar).contains(dynamicInv) &&
            !visitedDynamic.contains(dynamicVar) &&
            !visitedDynamic.at(dynamicVar).contains(dynamicInv)) {
          if (!visitedDynamic.contains(dynamicVar)) {
            visitedDynamic.emplace(
                dynamicVar,
                std::unordered_set<invariantgraph::VariableNode*>{});
          }
          visitedDynamic.at(dynamicVar).emplace(dynamicInv);
        }
      }
    }
    // add unvisited static neighbours to queue:
    for (auto* const staticInv : curVar->staticInputFor()) {
      for (auto* const staticVar : staticInv->definedVariables()) {
        if (!visitedLocal.contains(staticVar) &&
            !visitedLocal.at(staticVar).contains(staticInv)) {
          if (!visitedLocal.contains(staticVar)) {
            visitedLocal.emplace(
                staticVar, std::unordered_set<invariantgraph::VariableNode*>{});
          }
          visitedLocal.at(staticVar).emplace(staticInv);
          q.push(staticVar);
        }
      }
    }
  }
  return visitedDynamic;
}

static std::pair<invariantgraph::VariableNode*, invariantgraph::InvariantNode*>
findCycleUtil(
    invariantgraph::VariableNode* const variable,
    invariantgraph::InvariantNode* const definingInvariant,
    std::unordered_map<invariantgraph::VariableNode*,
                       std::unordered_set<invariantgraph::InvariantNode*>>&
        visitedGlobal,
    std::unordered_map<invariantgraph::VariableNode*,
                       std::unordered_set<invariantgraph::InvariantNode*>>&
        visitedLocal,
    std::unordered_map<
        invariantgraph::VariableNode*,
        std::unordered_map<invariantgraph::InvariantNode*,
                           std::pair<invariantgraph::VariableNode*,
                                     invariantgraph::InvariantNode*>>>& path) {
  if (visitedGlobal.contains(variable) &&
      visitedGlobal.at(variable).contains(definingInvariant)) {
    return std::pair<invariantgraph::VariableNode*,
                     invariantgraph::InvariantNode*>{nullptr, nullptr};
  }
  assert(!visitedLocal.contains(variable));
  visitedLocal.at(variable).emplace(definingInvariant);

  for (auto* const listeningInvariant : variable->staticInputFor()) {
    for (auto* const definedVariable : listeningInvariant->definedVariables()) {
      if (!visitedGlobal.contains(definedVariable) &&
          !visitedGlobal.at(definedVariable).contains(listeningInvariant) &&
          !visitedLocal.contains(definedVariable) &&
          !visitedLocal.at(definedVariable).contains(listeningInvariant)) {
        if (!path.contains(definedVariable)) {
          path.emplace(
              definedVariable,
              std::unordered_map<invariantgraph::InvariantNode*,
                                 std::pair<invariantgraph::VariableNode*,
                                           invariantgraph::InvariantNode*>>{});
        }
        if (!path.at(definedVariable).contains(listeningInvariant)) {
          path.at(definedVariable)
              .emplace(listeningInvariant,
                       std::pair<invariantgraph::VariableNode*,
                                 invariantgraph::InvariantNode*>{
                           variable, definingInvariant});
        }
        const auto cycleRoot =
            findCycleUtil(definedVariable, listeningInvariant, visitedGlobal,
                          visitedLocal, path);
        if (cycleRoot.first != nullptr && cycleRoot.second != nullptr) {
          return cycleRoot;
        }
      } else if (path.contains(definedVariable) &&
                 path.at(definedVariable).contains(listeningInvariant)) {
        return std::pair<invariantgraph::VariableNode*,
                         invariantgraph::InvariantNode*>{definedVariable,
                                                         listeningInvariant};
      }
    }
  }
  if (path.contains(variable)) {
    path.at(variable).erase(definingInvariant);
  }
  return std::pair<invariantgraph::VariableNode*,
                   invariantgraph::InvariantNode*>{nullptr, nullptr};
}

std::vector<invariantgraph::VariableNode*>
invariantgraph::InvariantGraph::breakCycles(
    VariableNode* variable,
    std::unordered_map<VariableNode*, std::unordered_set<InvariantNode*>>&
        visitedGlobal) {
  assert(variable->isSearchVariable());
  std::unordered_map<
      VariableNode*,
      std::unordered_map<InvariantNode*,
                         std::pair<VariableNode*, InvariantNode*>>>
      path;
  std::unordered_map<VariableNode*, std::unordered_set<InvariantNode*>>
      visitedLocal;
  std::pair<VariableNode*, InvariantNode*> cycleRoot{nullptr, nullptr};
  std::vector<VariableNode*> newSearchNodes;
  do {
    path.clear();
    visitedLocal.clear();
    cycleRoot =
        findCycleUtil(variable, nullptr, visitedGlobal, visitedLocal, path);
    if (cycleRoot.first != nullptr && cycleRoot.second != nullptr) {
      std::vector<std::pair<VariableNode*, InvariantNode*>> cycle;
      VariableNode* curVar = cycleRoot.first;
      InvariantNode* curInv = cycleRoot.second;
      while (path.contains(curVar) && path.at(curVar).contains(curInv)) {
        cycle.emplace_back(curVar, curInv);
        VariableNode* const tmp = curVar;
        curVar = path.at(tmp).at(curInv).first;
        curInv = path.at(tmp).at(curInv).second;
      }
      assert(std::all_of(cycle.begin(), cycle.end(), [&](const auto e) {
        return e.first != curVar && e.second != curInv;
      }));
      cycle.emplace_back(curVar, curInv);
      newSearchNodes.emplace_back(breakCycle(cycle));
      assert(newSearchNodes.back() != nullptr);
    }
  } while (cycleRoot.first != nullptr && cycleRoot.second);

  // add the visited nodes to the global set of visited nodes
  for (const auto& [visitedVariable, visitedInvariants] : visitedLocal) {
    for (auto* const visitedInvariant : visitedInvariants) {
      if (!visitedGlobal.contains(visitedVariable)) {
        visitedGlobal.emplace(visitedVariable,
                              std::unordered_set<InvariantNode*>{});
      }
      assert(!visitedGlobal.at(visitedVariable).contains(visitedInvariant));
      visitedGlobal.at(visitedVariable).emplace(visitedInvariant);
    }
  }
  return newSearchNodes;
}

void invariantgraph::InvariantGraph::breakCycles() {
  std::unordered_map<VariableNode*, std::unordered_set<InvariantNode*>>
      visitedGlobal;
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
  std::unordered_set<invariantgraph::InvariantNode*> visitedNodes;
  std::unordered_set<invariantgraph::VariableNode*> searchVariables;

  std::queue<invariantgraph::InvariantNode*> unregisteredNodes;

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
           node->staticInputs().front()->inputVarId() != NULL_ID);
#ifndef NDEBUG
    for (auto* const definedVariable : node->definedVariables()) {
      assert(definedVariable->inputVarId() == NULL_ID);
    }
#endif
    node->createDefinedVariables(engine);
    for (auto* const definedVariable : node->definedVariables()) {
      assert(definedVariable->isDefinedBy(node));
      definedVariables.emplace(definedVariable);
      assert(definedVariable->inputVarId() != NULL_ID);
      for (auto* const variableNode : definedVariable->inputFor()) {
        if (!visitedNodes.contains(variableNode)) {
          visitedNodes.emplace(variableNode);
          unregisteredNodes.emplace(variableNode);
        }
      }
    }
  }

  for (auto* const valueNode : _valueNodes) {
    if (valueNode->inputVarId() == NULL_ID) {
      auto constant = valueNode->constantValue();
      assert(constant);
      auto varId = engine.makeIntVar(*constant, *constant, *constant);
      valueNode->setVarId(varId);
    }
  }

#ifndef NDEBUG
  for (auto& node : _invariantNodes) {
    assert(visitedNodes.contains(node.get()));
  }
#endif
}

void invariantgraph::InvariantGraph::createInvariants(Engine& engine) {
  for (const auto& node : _invariantNodes) {
#ifndef NDEBUG
    for (auto* const definedVariable : node->definedVariables()) {
      assert(definedVariable->inputVarId() != NULL_ID);
    }
#endif
    node->registerWithEngine(engine);
  }
}

VarId invariantgraph::InvariantGraph::createViolations(Engine& engine) {
  std::vector<VarId> violations;
  for (const auto& definingNode : _invariantNodes) {
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
  engine.makeInvariant<Linear>(totalViolation, violations);
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
                             ? _objectiveVariable->inputVarId()
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
