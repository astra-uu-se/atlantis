#include "invariantgraph/invariantGraph.hpp"

#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/intEqNode.hpp"

namespace invariantgraph {

InvariantGraphRoot& InvariantGraph::root() {
  return dynamic_cast<InvariantGraphRoot&>(*_implicitConstraintNodes.front());
}

InvariantGraph::InvariantGraph()
    : _varNodes{VarNode(VarNodeId{1}, SearchDomain({1}), false),
                VarNode(VarNodeId{2}, SearchDomain({0}), false)},
      _namedVariableNodeIndices(),
      _intVariableNodeIndices(),
      _boolVariableNodeIndices{VarNodeId{1}, VarNodeId{2}},
      _invariantNodes(),
      _implicitConstraintNodes{},
      _objectiveVarNodeId(NULL_NODE_ID),
      _outputVars(),
      _outputArrays() {
  _implicitConstraintNodes.emplace_back(std::make_unique<InvariantGraphRoot>());
  root().init(*this, InvariantNodeId(1, true));
}

VarNodeId InvariantGraph::createVarNode(bool b) {
  return VarNodeId(_boolVariableNodeIndices.at(b ? 0 : 1));
}

VarNodeId InvariantGraph::createVarNode(const fznparser::BoolVar& var) {
  if (!var.identifier().empty() &&
      _namedVariableNodeIndices.contains(var.identifier())) {
    return _namedVariableNodeIndices.at(var.identifier());
  }

  auto nodeId =
      _varNodes.emplace_back(_varNodes.size() + 1, VarNode::FZNVariable(var))
          .varNodeId();

  if (!var.identifier().empty()) {
    _namedVariableNodeIndices.emplace(var.identifier(), nodeId);
  }

  if (var.isOutput() && !var.identifier().empty()) {
    _outputVars.emplace(var.identifier(), nodeId);
  }

  return nodeId;
}

VarNodeId InvariantGraph::createVarNode(
    std::reference_wrapper<const fznparser::BoolVar> ref) {
  return createVarNode(ref.get());
}

VarNodeId InvariantGraph::createVarNode(const fznparser::BoolArg& arg) {
  return arg.isParameter() ? createVarNode(arg.parameter())
                           : createVarNode(arg.var());
}

VarNodeId InvariantGraph::createVarNode(Int i) {
  if (_intVariableNodeIndices.contains(i)) {
    return _intVariableNodeIndices.at(i);
  }

  const VarNodeId nodeId = _varNodes
                               .emplace_back(VarNodeId(_varNodes.size() + 1),
                                             SearchDomain({i}), true)
                               .varNodeId();
  _intVariableNodeIndices.emplace(i, nodeId);
  return nodeId;
}

VarNodeId InvariantGraph::createVarNode(const fznparser::IntVar& var) {
  if (!var.identifier().empty() &&
      _namedVariableNodeIndices.contains(var.identifier())) {
    return _namedVariableNodeIndices.at(var.identifier());
  }

  const VarNodeId nodeId = _varNodes
                               .emplace_back(VarNodeId(_varNodes.size() + 1),
                                             VarNode::FZNVariable(var))
                               .varNodeId();
  if (!var.identifier().empty()) {
    _namedVariableNodeIndices.emplace(var.identifier(), nodeId);
  }

  if (var.isOutput() && !var.identifier().empty()) {
    _outputVars.emplace(var.identifier(), nodeId);
  }

  return nodeId;
}

VarNodeId InvariantGraph::createVarNode(
    std::reference_wrapper<const fznparser::IntVar> ref) {
  return createVarNode(ref);
}

VarNodeId InvariantGraph::createVarNode(const fznparser::IntArg& arg) {
  return arg.isParameter() ? createVarNode(arg.parameter())
                           : createVarNode(arg.var());
}

VarNodeId InvariantGraph::createVarNode(const SearchDomain& domain,
                                        bool isIntVar) {
  if (domain.isFixed()) {
    if (isIntVar) {
      return createVarNode(domain.lowerBound());
    } else {
      return createVarNode(domain.lowerBound() == 0);
    }
  }
  return _varNodes
      .emplace_back(VarNodeId(_varNodes.size() + 1), domain, isIntVar)
      .varNodeId();
  ;
}

std::vector<VarNodeId> InvariantGraph::createVarNodes(
    const fznparser::BoolVarArray& array) {
  std::vector<VarNodeId> variableNodes;
  variableNodes.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    variableNodes.emplace_back(
        std::holds_alternative<bool>(array.at(i))
            ? createVarNode(std::get<bool>(array.at(i)))
            : createVarNode(
                  std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                      array.at(i))
                      .get()));
  }

  if (array.isOutput() && !array.identifier().empty()) {
    _outputArrays.emplace(array.identifier(), variableNodes);
  }

  return variableNodes;
}

std::vector<VarNodeId> InvariantGraph::createVarNodes(
    const fznparser::IntVarArray& array) {
  std::vector<VarNodeId> variableNodes;
  variableNodes.reserve(array.size());

  for (size_t i = 0; i < array.size(); ++i) {
    variableNodes.emplace_back(
        std::holds_alternative<Int>(array.at(i))
            ? createVarNode(std::get<Int>(array.at(i)))
            : createVarNode(
                  std::get<std::reference_wrapper<const fznparser::IntVar>>(
                      array.at(i))
                      .get()));
  }

  if (array.isOutput() && !array.identifier().empty()) {
    _outputArrays.emplace(array.identifier(), variableNodes);
  }

  return variableNodes;
}

VarNode& InvariantGraph::varNode(const std::string& identifier) {
  assert(_namedVariableNodeIndices.contains(identifier));
  assert(_namedVariableNodeIndices.at(identifier).id != 0 &&
         _namedVariableNodeIndices.at(identifier).id <= _varNodes.size());
  return _varNodes.at(_namedVariableNodeIndices.at(identifier).id - 1);
}

VarNode& InvariantGraph::varNode(VarNodeId id) {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1);
}

VarId InvariantGraph::varId(const std::string& identifier) const {
  return _varNodes.at(_namedVariableNodeIndices.at(identifier).id - 1).varId();
}

VarId InvariantGraph::varId(VarNodeId id) const {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1).varId();
}

InvariantNode& InvariantGraph::invariantNode(InvariantNodeId id) {
  if (id.type == InvariantNodeId::Type::IMPLICIT_CONSTRAINT) {
    return implicitConstraintNode(id);
  }
  assert(id.id != 0 && id.id <= _invariantNodes.size());
  assert(id.type == InvariantNodeId::Type::INVARIANT);
  return *_invariantNodes.at(id.id - 1);
}

ImplicitConstraintNode& InvariantGraph::implicitConstraintNode(
    InvariantNodeId id) {
  assert(id.id != 0 && id.id <= _implicitConstraintNodes.size());
  assert(id.type == InvariantNodeId::Type::IMPLICIT_CONSTRAINT);
  return *_implicitConstraintNodes.at(id.id - 1);
}

InvariantNodeId InvariantGraph::nextInvariantNodeId() const noexcept {
  return InvariantNodeId(_invariantNodes.size());
}

InvariantNodeId InvariantGraph::nextImplicitNodeId() const noexcept {
  return InvariantNodeId(_implicitConstraintNodes.size(), true);
}

InvariantNodeId InvariantGraph::addInvariantNode(
    std::unique_ptr<InvariantNode>&& node) {
  auto& invNode = _invariantNodes.emplace_back(std::move(node));
  invNode->init(*this, InvariantNodeId(_invariantNodes.size(), false));
  return invNode->id();
}

InvariantNodeId InvariantGraph::addImplicitConstraintNode(
    std::unique_ptr<ImplicitConstraintNode>&& node) {
  auto& implNode = _implicitConstraintNodes.emplace_back(std::move(node));
  implNode->init(*this, InvariantNodeId(_implicitConstraintNodes.size(), true));
  return implNode->id();
}

void InvariantGraph::populateRootNode() {
  for (VarNode& var : _varNodes) {
    if (var.definingNodes().empty() && !var.inputTo().empty() &&
        !var.isFixed()) {
      root().addSearchVarNode(var);
    }
  }
}

void InvariantGraph::splitMultiDefinedVariables() {
  // DO NOT empace to _varNodes while doing this kind of iteration!

  size_t newSize = 0;
  for (const auto& varNode : _varNodes) {
    newSize += std::max<size_t>(1, varNode.definingNodes().size());
  }
  _varNodes.reserve(newSize);

  const size_t end = _varNodes.size();

  for (size_t i = 0; i < end; i++) {
    if (_varNodes[i].definingNodes().size() <= 1) {
      continue;
    }

    std::vector<InvariantNodeId> replacedDefiningNodes;
    std::vector<VarNodeId> splitNodes;
    replacedDefiningNodes.reserve(_varNodes[i].definingNodes().size() - 1);
    splitNodes.reserve(_varNodes[i].definingNodes().size());
    splitNodes.emplace_back(_varNodes[i].varNodeId());

    bool isFirstInvNodeId = true;
    for (const InvariantNodeId defInvNodeId : _varNodes[i].definingNodes()) {
      if (isFirstInvNodeId) {
        isFirstInvNodeId = false;
      } else {
        replacedDefiningNodes.emplace_back(defInvNodeId);
      }
    }

    for (const auto invNodeId : replacedDefiningNodes) {
      const VarNodeId newNodeId =
          createVarNode(_varNodes[i].constDomain(), _varNodes[i].isIntVar());
      splitNodes.emplace_back(newNodeId);
      invariantNode(invNodeId).replaceDefinedVariable(_varNodes[i],
                                                      varNode(newNodeId));
    }

    if (_varNodes[i].isIntVar()) {
      if (splitNodes.size() == 2) {
        addInvariantNode(std::move(std::make_unique<IntEqNode>(
            splitNodes.front(), splitNodes.back())));
      } else {
        addInvariantNode(
            std::move(std::make_unique<AllEqualNode>(std::move(splitNodes))));
      }
    } else if (splitNodes.size() == 2) {
      addInvariantNode(std::move(
          std::make_unique<BoolEqNode>(splitNodes.front(), splitNodes.back())));
    } else {
      addInvariantNode(
          std::move(std::make_unique<BoolAllEqualNode>(std::move(splitNodes))));
    }
  }
  assert(_varNodes.size() == newSize);
}

std::pair<VarNodeId, InvariantNodeId> InvariantGraph::findPivotInCycle(
    const std::vector<VarNodeId>& cycle) {
  assert(cycle.size() >= 1);
  VarNodeId pivot{NULL_NODE_ID};
  InvariantNodeId listeningInvariant{NULL_NODE_ID};
  size_t maxDomainSize = 0;
  for (size_t i = 0; i < cycle.size(); ++i) {
    if (varNode(cycle[i]).domain().size() > maxDomainSize) {
      maxDomainSize = varNode(cycle[i]).domain().size();
      pivot = cycle[i];
      listeningInvariant = varNode(cycle[(i + 1) % cycle.size()]).outputOf();
      assert(std::any_of(
          invariantNode(listeningInvariant).staticInputVarNodeIds().begin(),
          invariantNode(listeningInvariant).staticInputVarNodeIds().end(),
          [&](VarNodeId input) { return input == pivot; }));
    }
  }
  assert(pivot != NULL_NODE_ID);
  assert(maxDomainSize > 0);
  return std::pair<VarNodeId, InvariantNodeId>{pivot, listeningInvariant};
}

std::vector<VarNodeId> InvariantGraph::findCycle(
    const std::unordered_map<VarNodeId, VarNodeId, VarNodeIdHash>& childOf,
    const VarNodeId node, VarNodeId parent) {
  std::vector<VarNodeId> cycle;
  size_t i = 0;
  for (VarNodeId cur = parent; cur != node; cur = childOf.at(cur)) {
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

VarNodeId InvariantGraph::breakCycle(const std::vector<VarNodeId>& cycle) {
  const auto [pivot, listeningInvariant] = findPivotInCycle(cycle);

  assert(pivot != NULL_NODE_ID);
  assert(!varNode(pivot).isFixed());

  VarNodeId newInputNode =
      createVarNode(varNode(pivot).domain(), varNode(pivot).isIntVar());

  invariantNode(listeningInvariant)
      .replaceStaticInputVarNode(varNode(pivot), varNode(newInputNode));
  addInvariantNode(std::move(std::make_unique<IntEqNode>(pivot, newInputNode)));
  root().addSearchVarNode(varNode(newInputNode));
  return newInputNode;
}

std::unordered_set<VarNodeId, VarNodeIdHash>
InvariantGraph::dynamicVarNodeFrontier(
    const VarNodeId node,
    const std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal) {
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedLocal;
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedDynamic;
  std::queue<VarNodeId> q;
  q.push(node);
  visitedLocal.emplace(node);
  while (!q.empty()) {
    const auto cur = q.front();
    q.pop();
    // Add unvisited dynamic neighbours to set of visited dynamic variable
    // nodes:
    for (const auto dynamicInvNodeId : varNode(cur).dynamicInputTo()) {
      for (const auto outputVarId :
           invariantNode(dynamicInvNodeId).outputVarNodeIds()) {
        if (!visitedGlobal.contains(outputVarId) &&
            !visitedDynamic.contains(outputVarId)) {
          visitedDynamic.emplace(outputVarId);
        }
      }
    }
    // add unvisited static neighbours to queue:
    for (const auto staticInvNodeId : varNode(cur).staticInputTo()) {
      for (const auto outputVarId :
           invariantNode(staticInvNodeId).outputVarNodeIds()) {
        if (!visitedLocal.contains(outputVarId)) {
          visitedLocal.emplace(outputVarId);
          q.push(outputVarId);
        }
      }
    }
  }
  return visitedDynamic;
}

VarNodeId InvariantGraph::findCycleUtil(
    const VarNodeId varNodeId,
    const std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal,
    std::unordered_set<VarNodeId, VarNodeIdHash>& visitedLocal,
    std::unordered_map<VarNodeId, VarNodeId, VarNodeIdHash>& path) {
  if (visitedGlobal.contains(varNodeId)) {
    return VarNodeId(NULL_NODE_ID);
  }

  visitedLocal.emplace(varNodeId);
  for (const auto listeningInvNodeId : varNode(varNodeId).staticInputTo()) {
    for (const auto definedVarId :
         invariantNode(listeningInvNodeId).outputVarNodeIds()) {
      if (!visitedGlobal.contains(definedVarId) &&
          !visitedLocal.contains(definedVarId)) {
        path.emplace(varNodeId, definedVarId);
        const auto cycleRoot =
            findCycleUtil(definedVarId, visitedGlobal, visitedLocal, path);
        if (cycleRoot != NULL_NODE_ID) {
          return cycleRoot;
        }
      } else if (path.contains(definedVarId)) {
        return definedVarId;
      }
    }
  }
  path.erase(varNodeId);
  return VarNodeId(NULL_NODE_ID);
}

std::vector<VarNodeId> InvariantGraph::breakCycles(
    VarNodeId node,
    std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal) {
  std::unordered_map<VarNodeId, VarNodeId, VarNodeIdHash> path;
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedLocal;
  VarNodeId cycleRoot;
  std::vector<VarNodeId> newSearchNodes;
  do {
    path.clear();
    visitedLocal.clear();
    cycleRoot = findCycleUtil(node, visitedGlobal, visitedLocal, path);
    if (cycleRoot != NULL_NODE_ID) {
      std::vector<VarNodeId> cycle;
      VarNodeId cur = cycleRoot;
      for (; path.contains(cur); cur = path.at(cur)) {
        cycle.push_back(cur);
      }
      assert(std::all_of(cycle.begin(), cycle.end(),
                         [&](VarNodeId n) { return n != cur; }));
      cycle.push_back(cur);
      newSearchNodes.emplace_back(breakCycle(cycle));
      assert(newSearchNodes.back() != NULL_NODE_ID);
    }
  } while (cycleRoot != NULL_NODE_ID);

  // add the visited nodes to the global set of visited nodes
  for (const auto visitedNodeId : visitedLocal) {
    assert(!visitedGlobal.contains(visitedNodeId));
    visitedGlobal.emplace(visitedNodeId);
  }
  return newSearchNodes;
}

void InvariantGraph::breakCycles() {
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedGlobal;
  std::queue<VarNodeId> searchNodeIds;

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    for (const auto searchVarId : implicitConstraint->outputVarNodeIds()) {
      searchNodeIds.emplace(searchVarId);
    }
  }

  while (!searchNodeIds.empty()) {
    std::vector<VarNodeId> visitedSearchNodes;
    while (!searchNodeIds.empty()) {
      VarNodeId searchNodeId = searchNodeIds.front();
      searchNodeIds.pop();
      visitedSearchNodes.emplace_back(searchNodeId);
      for (auto newSearchNodeId : breakCycles(searchNodeId, visitedGlobal)) {
        searchNodeIds.emplace(newSearchNodeId);
        assert(varNode(newSearchNodeId).outputOf() == root().id());
      }
    }
    // If some part of the invariant graph is unreachable if not traversing
    // dynamic inputs of dynamic invariants, then add those dynamic inputs to
    // the set of search nodes:
    std::unordered_set<VarNodeId, VarNodeIdHash> dynamicSearchNodeIds;
    for (auto searchNodeId : visitedSearchNodes) {
      for (auto dynamicVarNodeId :
           dynamicVarNodeFrontier(searchNodeId, visitedGlobal)) {
        assert(!visitedGlobal.contains(dynamicVarNodeId));
        if (!dynamicSearchNodeIds.contains(dynamicVarNodeId)) {
          searchNodeIds.emplace(dynamicVarNodeId);
          dynamicSearchNodeIds.emplace(dynamicVarNodeId);
        }
      }
    }
  }
}

void InvariantGraph::createVariables(Engine& engine) {
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> visitedInvNodes;
  std::unordered_set<VarNodeId, VarNodeIdHash> searchVariables;

  std::queue<InvariantNodeId> unregisteredInvNodes;

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    visitedInvNodes.emplace(implicitConstraint->id());
    unregisteredInvNodes.emplace(implicitConstraint->id());
    for (const auto searchVarNodeId : implicitConstraint->outputVarNodeIds()) {
      searchVariables.emplace(searchVarNodeId);
    }
  }

  std::vector<VarId> violations;
  std::unordered_set<VarNodeId, VarNodeIdHash> outputVarNodeIds;

  while (!unregisteredInvNodes.empty()) {
    const auto invNodeId = unregisteredInvNodes.front();
    unregisteredInvNodes.pop();
    assert(visitedInvNodes.contains(invNodeId));
    // If the invNodeId only defines a single variable, then it is a view:
    assert(!invariantNode(invNodeId).dynamicInputVarNodeIds().empty() ||
           invariantNode(invNodeId).staticInputVarNodeIds().size() != 1 ||
           varId(invariantNode(invNodeId).staticInputVarNodeIds().front()) !=
               NULL_ID);
    assert(std::all_of(invariantNode(invNodeId).outputVarNodeIds().begin(),
                       invariantNode(invNodeId).outputVarNodeIds().end(),
                       [&](VarNodeId outputVarNodeId) {
                         return varId(outputVarNodeId) == NULL_ID;
                       }));

    invariantNode(invNodeId).registerOutputVariables(*this, engine);
    for (const auto outputVarNodeId :
         invariantNode(invNodeId).outputVarNodeIds()) {
      assert(outputVarNodeId != NULL_NODE_ID);
      const auto& vn = varNode(outputVarNodeId);
      assert(std::any_of(
          vn.definingNodes().begin(), vn.definingNodes().end(),
          [&](InvariantNodeId defNodeId) { return defNodeId == invNodeId; }));

      outputVarNodeIds.emplace(outputVarNodeId);
      assert(varId(outputVarNodeId) != NULL_ID);
      for (const auto nextVarDefNode : vn.inputTo()) {
        if (!visitedInvNodes.contains(nextVarDefNode)) {
          visitedInvNodes.emplace(nextVarDefNode);
          unregisteredInvNodes.emplace(nextVarDefNode);
        }
      }
    }
  }

  for (const auto varNodeId : _boolVariableNodeIndices) {
    if (varId(varNodeId) == NULL_ID && !varNode(varNodeId).inputTo().empty()) {
      auto constant = varNode(varNodeId).constantValue();
      assert(constant.has_value());
      VarId varId = engine.makeIntVar(constant.value(), constant.value(),
                                      constant.value());
      varNode(varNodeId).setVarId(varId);
    }
  }

  for (const auto& [constant, varNodeId] : _intVariableNodeIndices) {
    if (varId(varNodeId) == NULL_ID && !varNode(varNodeId).inputTo().empty()) {
      assert(varNode(varNodeId).constantValue().has_value() &&
             varNode(varNodeId).constantValue().value() == constant);
      VarId varId = engine.makeIntVar(constant, constant, constant);
      varNode(varNodeId).setVarId(varId);
    }
  }

  assert(std::all_of(_invariantNodes.begin(), _invariantNodes.end(),
                     [&](const auto& invNode) {
                       return visitedInvNodes.contains(invNode->id());
                     }));
}

void InvariantGraph::createImplicitConstraints(Engine& engine) {
  for (auto& implicitConstraintNode : _implicitConstraintNodes) {
    assert(std::all_of(
        implicitConstraintNode->outputVarNodeIds().begin(),
        implicitConstraintNode->outputVarNodeIds().end(),
        [&](VarNodeId varNodeId) { return varId(varNodeId) != NULL_ID; }));
    implicitConstraintNode->registerNode(*this, engine);
  }
}

void InvariantGraph::createInvariants(Engine& engine) {
  for (auto& invariantNode : _invariantNodes) {
    assert(std::all_of(
        invariantNode->outputVarNodeIds().begin(),
        invariantNode->outputVarNodeIds().end(),
        [&](VarNodeId varNodeId) { return varId(varNodeId) != NULL_ID; }));
    invariantNode->registerNode(*this, engine);
  }
}

VarId InvariantGraph::createViolations(Engine& engine) {
  std::vector<VarId> violations;
  for (const auto& definingNode : _invariantNodes) {
    if (!definingNode->isReified() &&
        definingNode->violationVarId(*this) != NULL_ID) {
      violations.emplace_back(definingNode->violationVarId(*this));
    }
  }

  std::unordered_set<VarNodeId, VarNodeIdHash> searchVariables;
  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    for (const auto& searchVarNodeId : implicitConstraint->outputVarNodeIds()) {
      searchVariables.emplace(searchVarNodeId);
    }
  }
  for (VarNode& var : _varNodes) {
    if (!searchVariables.contains(var.varNodeId()) && !var.inputTo().empty()) {
      const VarId violationId =
          var.postDomainConstraint(engine, var.constrainedDomain(engine));
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

InvariantGraphApplyResult InvariantGraph::apply(Engine& engine) {
  populateRootNode();
  splitMultiDefinedVariables();
  breakCycles();
  engine.open();
  createVariables(engine);
  createImplicitConstraints(engine);
  createInvariants(engine);
  engine.computeBounds();
  const VarId totalViolation = createViolations(engine);
  // If the model has no variable to optimise, use a dummy variable.
  VarId objectiveVarId = _objectiveVarNodeId != NULL_NODE_ID
                             ? varId(_objectiveVarNodeId)
                             : engine.makeIntVar(0, 0, 0);
  engine.close();

  InvariantGraphApplyResult::VariableIdentifiers variableIdentifiers;
  variableIdentifiers.reserve(_varNodes.size() +
                              (_objectiveVarNodeId != NULL_NODE_ID ? 1 : 0));

  for (const VarNode& node : _varNodes) {
    if (node.variable().has_value()) {
      assert(!node.identifier().empty());
      variableIdentifiers.emplace(node.varId(), node.identifier());
    }
  }
  if (_objectiveVarNodeId != NULL_NODE_ID &&
      varNode(_objectiveVarNodeId).variable().has_value()) {
    assert(!varNode(_objectiveVarNodeId).identifier().empty());
    variableIdentifiers.emplace(varId(_objectiveVarNodeId),
                                varNode(_objectiveVarNodeId).identifier());
  }

  return InvariantGraphApplyResult(std::move(variableIdentifiers),
                                   std::move(_implicitConstraintNodes),
                                   totalViolation, objectiveVarId);
}

}  // namespace invariantgraph