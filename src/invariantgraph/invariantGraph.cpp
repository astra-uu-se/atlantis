#include "invariantgraph/invariantGraph.hpp"

#include "invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "invariantgraph/violationInvariantNodes/intEqNode.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot& InvariantGraph::root() {
  return dynamic_cast<InvariantGraphRoot&>(*_implicitConstraintNodes.front());
}

InvariantGraph::InvariantGraph()
    : _varNodes{{VarNode(VarNodeId{1}, false, SearchDomain({1})), -1},
                {VarNode(VarNodeId{2}, false, SearchDomain({0})), -1}},
      _namedVarNodeIndices(),
      _intVarNodeIndices(),
      _boolVarNodeIndices{VarNodeId{1}, VarNodeId{2}},
      _invariantNodes(),
      _implicitConstraintNodes{},
      _totalViolationVarId(propagation::NULL_ID),
      _objectiveVarNodeId(NULL_NODE_ID) {
  addImplicitConstraintNode(std::make_unique<InvariantGraphRoot>());
}

InvariantGraph::VarNodeData& InvariantGraph::varNodeData(VarNodeId id) {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1);
}

InvariantGraph::VarNodeData& InvariantGraph::varNodeData(
    const std::string& identifier) {
  assert(_namedVarNodeIndices.contains(identifier));
  assert(_namedVarNodeIndices.at(identifier).id != 0 &&
         _namedVarNodeIndices.at(identifier).id <= _varNodes.size());
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1);
}

Int InvariantGraph::markDuplicate(VarNodeId id, const std::string& identifier) {
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, id);
    return -1;
  } else if (varNode(identifier).varNodeId() == id) {
    return -1;
  }
  auto& originalVarNodeData = varNodeData(identifier);

  if (originalVarNodeData.duplicationIndex != -1) {
    varNodeData(id).duplicationIndex = originalVarNodeData.duplicationIndex;
    return originalVarNodeData.duplicationIndex;
  }
  Int duplicationIndex = _duplicateVarNodes.size();
  originalVarNodeData.duplicationIndex = duplicationIndex;
  varNodeData(id).duplicationIndex = duplicationIndex;
  _duplicateVarNodes.emplace_back(
      std::vector<VarNodeId>{originalVarNodeData.varNode.varNodeId(), id});
  return duplicationIndex;
}

VarNodeId InvariantGraph::nextVarNodeId() const noexcept {
  return VarNodeId(_varNodes.size() + 1);
}

bool InvariantGraph::containsVarNode(
    const std::string& identifier) const noexcept {
  return _namedVarNodeIndices.contains(identifier);
}

bool InvariantGraph::containsVarNode(Int i) const noexcept {
  return _intVarNodeIndices.contains(i);
}

bool InvariantGraph::containsVarNode(bool) noexcept { return true; }

VarNodeId InvariantGraph::inputBoolVarNode(bool b) {
  return _boolVarNodeIndices.at(b ? 0 : 1);
}

VarNodeId InvariantGraph::inputBoolVarNode(const std::string& identifier) {
  if (!containsVarNode(identifier)) {
    throw std::invalid_argument("No variable with identifier " + identifier);
  }
  assert(!varNode(identifier).isIntVar());
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::defineBoolVarNode() {
  return _varNodes
      .emplace_back(VarNode{nextVarNodeId(), true, SearchDomain(0, 1)}, 1)
      .varNode.varNodeId();
}

VarNodeId InvariantGraph::defineBoolVarNode(const std::string& identifier) {
  const VarNodeId nId = defineBoolVarNode();
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, nId);
  } else {
    markDuplicate(nId, identifier);
  }
  return nId;
}

VarNodeId InvariantGraph::defineBoolVarNode(bool b) {
  return _varNodes
      .emplace_back(VarNode{nextVarNodeId(), false, SearchDomain({b ? 0 : 1})},
                    -1)
      .varNode.varNodeId();
}

VarNodeId InvariantGraph::defineBoolVarNode(bool b,
                                            const std::string& identifier) {
  const VarNodeId nId = defineBoolVarNode(b);
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, nId);
  } else {
    markDuplicate(nId, identifier);
  }
  return nId;
}

VarNodeId InvariantGraph::defineBoolVarNode(SearchDomain&& domain) {
  if (domain.isFixed()) {
    return defineBoolVarNode(domain.lowerBound() == 0);
  }
  return _varNodes
      .emplace_back(VarNode{nextVarNodeId(), true, std::move(domain)}, -1)
      .varNode.varNodeId();
}

VarNodeId InvariantGraph::inputIntVarNode(Int i) {
  if (!containsVarNode(i)) {
    const VarNodeId nodeId = defineIntVarNode(i);
    _intVarNodeIndices.emplace(i, nodeId);
    return nodeId;
  }
  return _intVarNodeIndices.at(i);
}

VarNodeId InvariantGraph::inputIntVarNode(const std::string& identifier) {
  if (!containsVarNode(identifier)) {
    throw std::invalid_argument("No variable with identifier " + identifier);
  }
  assert(varNode(identifier).isIntVar());
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::defineIntVarNode(Int i) {
  return _varNodes
      .emplace_back(VarNode{nextVarNodeId(), true, SearchDomain({i})}, -1)
      .varNode.varNodeId();
}

VarNodeId InvariantGraph::defineIntVarNode(Int i,
                                           const std::string& identifier) {
  const VarNodeId inputVarNodeId = defineIntVarNode(i);
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, inputVarNodeId);
  } else {
    markDuplicate(inputVarNodeId, identifier);
  }
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::defineIntVarNode() {
  return _varNodes.emplace_back(VarNode{nextVarNodeId(), true}, -1)
      .varNode.varNodeId();
}

VarNodeId InvariantGraph::defineIntVarNode(SearchDomain&& domain) {
  if (domain.isFixed()) {
    return defineIntVarNode(domain.lowerBound());
  }
  return _varNodes
      .emplace_back(VarNode{nextVarNodeId(), true, std::move(domain)}, -1)
      .varNode.varNodeId();
}

VarNodeId InvariantGraph::defineIntVarNode(SearchDomain&& domain,
                                           const std::string& identifier) {
  const VarNodeId nId = defineIntVarNode(std::move(domain));
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, nId);
  } else {
    markDuplicate(nId, identifier);
  }
  return nId;
}

VarNode& InvariantGraph::varNode(const std::string& identifier) {
  assert(_namedVarNodeIndices.contains(identifier));
  assert(_namedVarNodeIndices.at(identifier).id != 0 &&
         _namedVarNodeIndices.at(identifier).id <= _varNodes.size());
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1).varNode;
}

VarNode& InvariantGraph::varNode(VarNodeId id) {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1).varNode;
}

const VarNode& InvariantGraph::varNodeConst(
    const std::string& identifier) const {
  assert(_namedVarNodeIndices.contains(identifier));
  assert(_namedVarNodeIndices.at(identifier).id != 0 &&
         _namedVarNodeIndices.at(identifier).id <= _varNodes.size());
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1).varNode;
}

const VarNode& InvariantGraph::varNodeConst(VarNodeId id) const {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1).varNode;
}

VarNodeId InvariantGraph::varNodeId(const std::string& identifier) const {
  if (!containsVarNode(identifier)) {
    return VarNodeId(NULL_NODE_ID);
  }
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::varNodeId(bool val) const {
  if (!containsVarNode(val)) {
    return VarNodeId(NULL_NODE_ID);
  }
  return _boolVarNodeIndices.at(val);
}

VarNodeId InvariantGraph::varNodeId(Int val) const {
  if (!containsVarNode(val)) {
    return VarNodeId(NULL_NODE_ID);
  }
  return _intVarNodeIndices.at(val);
}

propagation::VarId InvariantGraph::varId(const std::string& identifier) const {
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1)
      .varNode.varId();
}

propagation::VarId InvariantGraph::varId(VarNodeId id) const {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1).varNode.varId();
}

bool InvariantGraph::containsInvariantNode(InvariantNodeId id) const noexcept {
  return id.type == InvariantNodeId::Type::INVARIANT &&
         id.id <= _invariantNodes.size();
}

bool InvariantGraph::containsImplicitConstraintNode(
    InvariantNodeId id) const noexcept {
  return id.type == InvariantNodeId::Type::IMPLICIT_CONSTRAINT &&
         id.id <= _implicitConstraintNodes.size();
}

InvariantNode& InvariantGraph::invariantNode(InvariantNodeId id) {
  if (id.type == InvariantNodeId::Type::IMPLICIT_CONSTRAINT) {
    return implicitConstraintNode(id);
  }
  assert(containsInvariantNode(id));
  return *_invariantNodes.at(id.id - 1);
}

ImplicitConstraintNode& InvariantGraph::implicitConstraintNode(
    InvariantNodeId id) {
  assert(containsImplicitConstraintNode(id));
  return *_implicitConstraintNodes.at(id.id - 1);
}

InvariantNodeId InvariantGraph::nextInvariantNodeId() const noexcept {
  return {_invariantNodes.size() + 1, false};
}

InvariantNodeId InvariantGraph::nextImplicitNodeId() const noexcept {
  return {_implicitConstraintNodes.size() + 1, true};
}

InvariantNodeId InvariantGraph::addInvariantNode(
    std::unique_ptr<InvariantNode>&& node) {
  const InvariantNodeId id = nextInvariantNodeId();
  auto& invNode = _invariantNodes.emplace_back(std::move(node));
  invNode->init(*this, id);
  return invNode->id();
}

InvariantNodeId InvariantGraph::addImplicitConstraintNode(
    std::unique_ptr<ImplicitConstraintNode>&& node) {
  const InvariantNodeId id = nextImplicitNodeId();
  auto& implNode = _implicitConstraintNodes.emplace_back(std::move(node));
  implNode->init(*this, id);
  return implNode->id();
}

search::neighbourhoods::NeighbourhoodCombinator InvariantGraph::neighbourhood()
    const noexcept {
  std::vector<std::shared_ptr<search::neighbourhoods::Neighbourhood>>
      neighbourhoods;
  neighbourhoods.reserve(_implicitConstraintNodes.size());

  for (auto const& implicitContraint : _implicitConstraintNodes) {
    std::shared_ptr<search::neighbourhoods::Neighbourhood> neighbourhood =
        implicitContraint->neighbourhood();
    if (neighbourhood != nullptr) {
      neighbourhoods.push_back(std::move(neighbourhood));
    }
  }

  return search::neighbourhoods::NeighbourhoodCombinator(
      std::move(neighbourhoods));
}

propagation::VarId InvariantGraph::totalViolationVarId() const noexcept {
  return _totalViolationVarId;
}

propagation::VarId InvariantGraph::objectiveVarId() const noexcept {
  return varId(_objectiveVarNodeId);
}

void InvariantGraph::populateRootNode() {
  for (VarNodeData& data : _varNodes) {
    if (data.varNode.definingNodes().empty() &&
        !data.varNode.inputTo().empty() && !data.varNode.isFixed()) {
      root().addSearchVarNode(data.varNode);
    }
  }
}

void InvariantGraph::splitMultiDefinedVars() {
  // DO NOT empace to _varNodes while doing this kind of iteration!

  size_t newSize = 0;
  for (const auto& data : _varNodes) {
    newSize += std::max<size_t>(1, data.varNode.definingNodes().size());
  }
  _varNodes.reserve(newSize);

  const size_t end = _varNodes.size();

  for (size_t i = 0; i < end; i++) {
    if (_varNodes[i].varNode.definingNodes().size() <= 1) {
      continue;
    }

    std::vector<InvariantNodeId> replacedDefiningNodes;
    std::vector<VarNodeId> splitNodes;
    replacedDefiningNodes.reserve(_varNodes[i].varNode.definingNodes().size() -
                                  1);
    splitNodes.reserve(_varNodes[i].varNode.definingNodes().size());
    splitNodes.emplace_back(_varNodes[i].varNode.varNodeId());

    bool isFirstInvNodeId = true;
    for (const InvariantNodeId& defInvNodeId :
         _varNodes[i].varNode.definingNodes()) {
      if (isFirstInvNodeId) {
        isFirstInvNodeId = false;
      } else {
        replacedDefiningNodes.emplace_back(defInvNodeId);
      }
    }

    for (const auto& invNodeId : replacedDefiningNodes) {
      const VarNodeId newNodeId =
          _varNodes[i].varNode.isIntVar()
              ? defineIntVarNode(
                    SearchDomain(_varNodes[i].varNode.constDomain()))
              : defineBoolVarNode(
                    SearchDomain(_varNodes[i].varNode.constDomain()));
      splitNodes.emplace_back(newNodeId);
      invariantNode(invNodeId).replaceDefinedVar(_varNodes[i].varNode,
                                                 varNode(newNodeId));
    }

    if (_varNodes[i].varNode.isIntVar()) {
      if (splitNodes.size() == 2) {
        addInvariantNode(
            std::make_unique<IntEqNode>(splitNodes.front(), splitNodes.back()));
      } else {
        addInvariantNode(
            std::make_unique<IntAllEqualNode>(std::move(splitNodes)));
      }
    } else if (splitNodes.size() == 2) {
      addInvariantNode(
          std::make_unique<BoolEqNode>(splitNodes.front(), splitNodes.back()));
    } else {
      addInvariantNode(
          std::make_unique<BoolAllEqualNode>(std::move(splitNodes)));
    }
  }
  assert(_varNodes.size() == newSize);
}

std::pair<VarNodeId, InvariantNodeId> InvariantGraph::findPivotInCycle(
    const std::vector<VarNodeId>& cycle) {
  assert(!cycle.empty());
  VarNodeId pivot{NULL_NODE_ID};
  InvariantNodeId listeningInvariant{NULL_NODE_ID};
  size_t maxDomainSize = 0;
  for (size_t i = 0; i < cycle.size(); ++i) {
    if (varNode(cycle[i]).constDomain().size() > maxDomainSize) {
      maxDomainSize = varNode(cycle[i]).constDomain().size();
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

VarNodeId InvariantGraph::breakCycle(const std::vector<VarNodeId>& cycle) {
  const auto [pivot, listeningInvariant] = findPivotInCycle(cycle);

  assert(pivot != NULL_NODE_ID);
  assert(!varNode(pivot).isFixed());

  VarNodeId newInputNode =
      varNode(pivot).isIntVar()
          ? defineIntVarNode(SearchDomain(varNode(pivot).constDomain()))
          : defineBoolVarNode(SearchDomain(varNode(pivot).constDomain()));

  invariantNode(listeningInvariant)
      .replaceStaticInputVarNode(varNode(pivot), varNode(newInputNode));
  addInvariantNode(std::make_unique<IntEqNode>(pivot, newInputNode));
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
    for (const auto& dynamicInvNodeId : varNode(cur).dynamicInputTo()) {
      for (const auto& outputVarId :
           invariantNode(dynamicInvNodeId).outputVarNodeIds()) {
        if (!visitedGlobal.contains(outputVarId) &&
            !visitedDynamic.contains(outputVarId)) {
          visitedDynamic.emplace(outputVarId);
        }
      }
    }
    // add unvisited static neighbours to queue:
    for (const auto& staticInvNodeId : varNode(cur).staticInputTo()) {
      for (const auto& outputVarId :
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
  for (const InvariantNodeId& listeningInvNodeId :
       varNode(varNodeId).staticInputTo()) {
    for (const VarNodeId& definedVarId :
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
  for (const auto& visitedNodeId : visitedLocal) {
    assert(!visitedGlobal.contains(visitedNodeId));
    visitedGlobal.emplace(visitedNodeId);
  }
  return newSearchNodes;
}

void InvariantGraph::breakCycles() {
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedGlobal;
  std::queue<VarNodeId> searchNodeIds;

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    for (const auto& searchVarId : implicitConstraint->outputVarNodeIds()) {
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

void InvariantGraph::createVars(propagation::SolverBase& solver) {
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> visitedInvNodes;
  std::unordered_set<VarNodeId, VarNodeIdHash> searchVars;

  std::queue<InvariantNodeId> unregisteredInvNodes;

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    visitedInvNodes.emplace(implicitConstraint->id());
    unregisteredInvNodes.emplace(implicitConstraint->id());
    for (const auto& searchVarNodeId : implicitConstraint->outputVarNodeIds()) {
      searchVars.emplace(searchVarNodeId);
    }
  }

  std::unordered_set<VarNodeId, VarNodeIdHash> outputVarNodeIds;

  while (!unregisteredInvNodes.empty()) {
    const auto invNodeId = unregisteredInvNodes.front();
    unregisteredInvNodes.pop();
    assert(visitedInvNodes.contains(invNodeId));
    // If the invNodeId only defines a single variable, then it is a view:
    assert(!invariantNode(invNodeId).dynamicInputVarNodeIds().empty() ||
           invariantNode(invNodeId).staticInputVarNodeIds().size() != 1 ||
           varId(invariantNode(invNodeId).staticInputVarNodeIds().front()) !=
               propagation::NULL_ID);
    assert(std::all_of(invariantNode(invNodeId).outputVarNodeIds().begin(),
                       invariantNode(invNodeId).outputVarNodeIds().end(),
                       [&](VarNodeId outputVarNodeId) {
                         return varId(outputVarNodeId) == propagation::NULL_ID;
                       }));

    invariantNode(invNodeId).registerOutputVars(*this, solver);
    for (const auto& outputVarNodeId :
         invariantNode(invNodeId).outputVarNodeIds()) {
      assert(outputVarNodeId != NULL_NODE_ID);
      const auto& vn = varNode(outputVarNodeId);
      assert(std::any_of(
          vn.definingNodes().begin(), vn.definingNodes().end(),
          [&](InvariantNodeId defNodeId) { return defNodeId == invNodeId; }));

      outputVarNodeIds.emplace(outputVarNodeId);
      assert(varId(outputVarNodeId) != propagation::NULL_ID);
      for (const auto& nextVarDefNode : vn.inputTo()) {
        if (!visitedInvNodes.contains(nextVarDefNode)) {
          visitedInvNodes.emplace(nextVarDefNode);
          unregisteredInvNodes.emplace(nextVarDefNode);
        }
      }
    }
  }

  for (const auto& varNodeId : _boolVarNodeIndices) {
    if (varId(varNodeId) == propagation::NULL_ID &&
        !varNode(varNodeId).inputTo().empty()) {
      auto constant = varNode(varNodeId).constantValue();
      assert(constant.has_value());
      propagation::VarId varId = solver.makeIntVar(
          constant.value(), constant.value(), constant.value());
      varNode(varNodeId).setVarId(varId);
    }
  }

  for (const auto& [constant, varNodeId] : _intVarNodeIndices) {
    if (varId(varNodeId) == propagation::NULL_ID &&
        !varNode(varNodeId).inputTo().empty()) {
      assert(varNode(varNodeId).constantValue().has_value() &&
             varNode(varNodeId).constantValue().value() == constant);
      propagation::VarId varId =
          solver.makeIntVar(constant, constant, constant);
      varNode(varNodeId).setVarId(varId);
    }
  }

  assert(std::all_of(_invariantNodes.begin(), _invariantNodes.end(),
                     [&](const auto& invNode) {
                       return visitedInvNodes.contains(invNode->id());
                     }));
}

void InvariantGraph::createImplicitConstraints(
    propagation::SolverBase& solver) {
  for (auto& implicitConstraintNode : _implicitConstraintNodes) {
    assert(std::all_of(implicitConstraintNode->outputVarNodeIds().begin(),
                       implicitConstraintNode->outputVarNodeIds().end(),
                       [&](VarNodeId varNodeId) {
                         return varId(varNodeId) != propagation::NULL_ID;
                       }));
    implicitConstraintNode->registerNode(*this, solver);
  }
}

void InvariantGraph::createInvariants(propagation::SolverBase& solver) {
  for (auto& invariantNode : _invariantNodes) {
    assert(std::all_of(invariantNode->outputVarNodeIds().begin(),
                       invariantNode->outputVarNodeIds().end(),
                       [&](VarNodeId varNodeId) {
                         return varId(varNodeId) != propagation::NULL_ID;
                       }));
    invariantNode->registerNode(*this, solver);
  }
}

propagation::VarId InvariantGraph::createViolations(
    propagation::SolverBase& solver) {
  std::vector<propagation::VarId> violations;
  for (const auto& definingNode : _invariantNodes) {
    if (!definingNode->isReified() &&
        definingNode->violationVarId(*this) != propagation::NULL_ID) {
      violations.emplace_back(definingNode->violationVarId(*this));
    }
  }

  for (VarNodeData& data : _varNodes) {
    if ((!data.varNode.inputTo().empty() ||
         !data.varNode.definingNodes().empty()) &&
        data.varNode.hasDomain()) {
      const propagation::VarId violationId = data.varNode.postDomainConstraint(
          solver, data.varNode.constrainedDomain(solver));
      if (violationId != propagation::NULL_ID) {
        violations.emplace_back(violationId);
      }
    }
  }
  if (violations.empty()) {
    return propagation::NULL_ID;
  }
  if (violations.size() == 1) {
    return violations.front();
  }
  const propagation::VarId totalViolation = solver.makeIntVar(0, 0, 0);
  solver.makeInvariant<propagation::Linear>(solver, totalViolation,
                                            std::move(violations));
  return totalViolation;
}

void InvariantGraph::apply(propagation::SolverBase& solver) {
  populateRootNode();
  splitMultiDefinedVars();
  breakCycles();
  solver.open();
  createVars(solver);
  createImplicitConstraints(solver);
  createInvariants(solver);
  solver.computeBounds();
  _totalViolationVarId = createViolations(solver);
  if (_objectiveVarNodeId == NULL_NODE_ID) {
    // We use the true Boolean fixed variable (any fixed variable will do):
    _objectiveVarNodeId = _boolVarNodeIndices[1];
    assert(_objectiveVarNodeId != NULL_NODE_ID);
  }
  if (varNode(_objectiveVarNodeId).varId() == propagation::NULL_ID) {
    varNode(_objectiveVarNodeId).setVarId(solver.makeIntVar(0, 0, 0));
  }
  solver.close();
}

}  // namespace atlantis::invariantgraph