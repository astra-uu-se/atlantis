#include "atlantis/invariantgraph/invariantGraph.hpp"

#include <deque>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/utils/fznAst.hpp"

namespace atlantis::invariantgraph {

InvariantGraphRoot& InvariantGraph::root() {
  return dynamic_cast<InvariantGraphRoot&>(*_implicitConstraintNodes.front());
}

InvariantGraph::InvariantGraph()
    : _varNodes{VarNode{VarNodeId{1}, false, SearchDomain({1})},
                VarNode{VarNodeId{2}, false, SearchDomain({0})}},
      _namedVarNodeIndices(),
      _intVarNodeIndices(),
      _boolVarNodeIndices{VarNodeId{1}, VarNodeId{2}},
      _invariantNodes(),
      _implicitConstraintNodes{},
      _totalViolationVarId(propagation::NULL_ID),
      _objectiveVarNodeId(NULL_NODE_ID) {
  addImplicitConstraintNode(std::make_unique<InvariantGraphRoot>());
}

VarNodeId InvariantGraph::nextVarNodeId() const noexcept {
  return VarNodeId(_varNodes.size() + 1);
}

bool InvariantGraph::containsVarNode(
    const std::string& identifier) const noexcept {
  return !_namedVarNodeIndices.empty() &&
         _namedVarNodeIndices.contains(identifier);
}

bool InvariantGraph::containsVarNode(Int i) const noexcept {
  return !_intVarNodeIndices.empty() && _intVarNodeIndices.contains(i);
}

bool InvariantGraph::containsVarNode(bool) noexcept { return true; }

VarNodeId InvariantGraph::retrieveBoolVarNode(bool b) {
  return _boolVarNodeIndices.at(b ? 0 : 1);
}

VarNodeId InvariantGraph::retrieveBoolVarNode(const std::string& identifier,
                                              VarNode::DomainType domainType) {
  if (!containsVarNode(identifier)) {
    const VarNodeId nId = retrieveBoolVarNode(domainType);
    _namedVarNodeIndices.emplace(identifier, nId);
    return nId;
  }
  assert(!varNode(identifier).isIntVar());
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::retrieveBoolVarNode(VarNode::DomainType domainType) {
  return _varNodes.emplace_back(nextVarNodeId(), false, domainType).varNodeId();
}

VarNodeId InvariantGraph::retrieveBoolVarNode(bool b,
                                              const std::string& identifier) {
  const VarNodeId nId = retrieveBoolVarNode(b);
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, nId);
  } else {
    auto& var = varNode(identifier);
    if (var.isIntVar()) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not a boolean variable");
    }
    if (!var.isFixed() || !var.inDomain(b)) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not fixed to " + (b ? "true" : "false"));
    }
  }
  return nId;
}

VarNodeId InvariantGraph::retrieveBoolVarNode(SearchDomain&& domain,
                                              VarNode::DomainType domainType) {
  if (domain.isFixed()) {
    return retrieveBoolVarNode(domain.lowerBound() == 0);
  }
  return _varNodes
      .emplace_back(nextVarNodeId(), false, std::move(domain), domainType)
      .varNodeId();
}

VarNodeId InvariantGraph::retrieveIntVarNode(Int value) {
  if (!containsVarNode(value)) {
    const VarNodeId nodeId =
        _varNodes
            .emplace_back(nextVarNodeId(), true, SearchDomain({value}),
                          VarNode::DomainType::FIXED)
            .varNodeId();
    _intVarNodeIndices.emplace(value, nodeId);
    return nodeId;
  } else {
    const auto& var = varNode(value);
    if (!var.isIntVar()) {
      throw std::invalid_argument("Variable " + std::to_string(value) +
                                  " is not an integer variable");
    }
    if (!var.isFixed() || !var.inDomain(value)) {
      throw std::invalid_argument("Variable " + std::to_string(value) +
                                  " is not fixed to " + std::to_string(value));
    }
  }
  return _intVarNodeIndices.at(value);
}

VarNodeId InvariantGraph::retrieveIntVarNode(const std::string& identifier) {
  if (!containsVarNode(identifier)) {
    throw std::invalid_argument("No variable with identifier " + identifier);
  }
  if (!varNode(identifier).isIntVar()) {
    throw std::invalid_argument("Variable " + identifier +
                                " is not an integer variable");
  }
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::retrieveIntVarNode(Int i,
                                             const std::string& identifier) {
  const VarNodeId inputVarNodeId = retrieveIntVarNode(i);
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, inputVarNodeId);
  }
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::retrieveIntVarNode(SearchDomain&& domain,
                                             VarNode::DomainType domainType) {
  if (domain.isFixed()) {
    return retrieveIntVarNode(domain.lowerBound());
  }
  return _varNodes
      .emplace_back(nextVarNodeId(), true, std::move(domain), domainType)
      .varNodeId();
}

VarNodeId InvariantGraph::retrieveIntVarNode(SearchDomain&& domain,
                                             const std::string& identifier,
                                             VarNode::DomainType domainType) {
  if (containsVarNode(identifier)) {
    const auto& node = varNode(identifier);
    if (!node.isIntVar()) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not an integer variable");
    }
    return node.varNodeId();
  }
  const VarNodeId nId = retrieveIntVarNode(std::move(domain), domainType);

  assert(!containsVarNode(identifier));
  _namedVarNodeIndices.emplace(identifier, nId);
  return nId;
}

VarNode& InvariantGraph::varNode(const std::string& identifier) {
  assert(_namedVarNodeIndices.contains(identifier));
  assert(_namedVarNodeIndices.at(identifier).id != 0 &&
         _namedVarNodeIndices.at(identifier).id <= _varNodes.size());
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1);
}

VarNode& InvariantGraph::varNode(VarNodeId id) {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1);
}

VarNode& InvariantGraph::varNode(Int value) {
  assert(_intVarNodeIndices.contains(value));
  assert(_intVarNodeIndices.at(value).id != 0 &&
         _intVarNodeIndices.at(value).id <= _varNodes.size());
  return _varNodes.at(_intVarNodeIndices.at(value).id - 1);
}

void InvariantGraph::replaceInvariantNodes() {
  size_t invIndex = 0;
  size_t implIndex = 0;
  while (invIndex < _invariantNodes.size() &&
         implIndex < _implicitConstraintNodes.size()) {
    while (invIndex < _invariantNodes.size()) {
      auto& invNode = *_invariantNodes.at(invIndex);
      invNode.updateState(*this);
      if (invNode.state() == InvariantNodeState::SUBSUMED) {
        invNode.deactivate(*this);
      } else {
        if (invNode.canBeReplaced(*this)) {
          if (invNode.replace(*this)) {
            invNode.deactivate(*this);
          }
        } else if (invNode.canBeMadeImplicit(*this)) {
          if (invNode.makeImplicit(*this)) {
            invNode.deactivate(*this);
          }
        }
      }
      ++invIndex;
    }
    while (implIndex < _implicitConstraintNodes.size()) {
      auto& implNode = *_implicitConstraintNodes.at(implIndex);
      implNode.updateState(*this);
      if (implNode.state() == InvariantNodeState::SUBSUMED) {
        implNode.deactivate(*this);
      } else {
        if (implNode.canBeReplaced(*this)) {
          if (implNode.replace(*this)) {
            implNode.deactivate(*this);
          }
        }
      }
      ++implIndex;
    }
  }
}

const VarNode& InvariantGraph::varNodeConst(
    const std::string& identifier) const {
  assert(_namedVarNodeIndices.contains(identifier));
  assert(_namedVarNodeIndices.at(identifier).id != 0 &&
         _namedVarNodeIndices.at(identifier).id <= _varNodes.size());
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1);
}

const VarNode& InvariantGraph::varNodeConst(VarNodeId id) const {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1);
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
  return _varNodes.at(_namedVarNodeIndices.at(identifier).id - 1).varId();
}

propagation::VarId InvariantGraph::varId(VarNodeId id) const {
  assert(id.id != 0 && id.id <= _varNodes.size());
  return _varNodes.at(id.id - 1).varId();
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

void InvariantGraph::replaceVarNode(VarNodeId oldNodeId, VarNodeId newNodeId) {
  if (oldNodeId == newNodeId) {
    return;
  }
  VarNode& oldNode = varNode(oldNodeId);
  VarNode& newNode = varNode(newNodeId);
  newNode.domain().intersect(oldNode.constDomain());
  while (!oldNode.definingNodes().empty()) {
    invariantNode(*(oldNode.definingNodes().begin()))
        .replaceDefinedVar(oldNode, newNode);
  }
  while (!oldNode.staticInputTo().empty()) {
    invariantNode(oldNode.staticInputTo().front())
        .replaceStaticInputVarNode(oldNode, newNode);
  }
  while (!oldNode.dynamicInputTo().empty()) {
    invariantNode(oldNode.dynamicInputTo().front())
        .replaceDynamicInputVarNode(oldNode, newNode);
  }
  if (oldNode.isFixed()) {
    if (oldNode.isIntVar() &&
        _intVarNodeIndices.contains(oldNode.lowerBound()) &&
        _intVarNodeIndices.at(oldNode.lowerBound()) == oldNodeId) {
      _intVarNodeIndices.erase(oldNode.lowerBound());
      _intVarNodeIndices.emplace(oldNode.lowerBound(), newNodeId);
    } else {
      size_t index = oldNode.lowerBound() == 0 ? 0 : 1;
      if (_boolVarNodeIndices.at(index) == oldNodeId) {
        _boolVarNodeIndices[index] = newNodeId;
      }
    }
  }
  for (auto& [identifier, id] : _namedVarNodeIndices) {
    if (id == oldNodeId) {
      id = newNodeId;
    }
  }
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
  for (auto& vNode : _varNodes) {
    if (vNode.definingNodes().empty() && !vNode.inputTo().empty() &&
        !vNode.isFixed()) {
      root().addSearchVarNode(vNode);
    }
  }
}

void InvariantGraph::splitMultiDefinedVars() {
  // DO NOT empace to _varNodes while doing this kind of iteration!

  size_t newSize = 0;
  for (const auto& vNode : _varNodes) {
    newSize += std::max<size_t>(
        1, vNode.definingNodes().size() + static_cast<size_t>(vNode.isFixed()));
  }
  _varNodes.reserve(newSize);

  const size_t end = _varNodes.size();

  for (size_t i = 0; i < end; i++) {
    const bool isFixed = _varNodes[i].isFixed();
    if (_varNodes[i].definingNodes().size() + static_cast<size_t>(isFixed) <=
        1) {
      continue;
    }

    std::vector<InvariantNodeId> replacedDefiningNodes;
    replacedDefiningNodes.reserve(_varNodes[i].definingNodes().size());

    bool isFirstInvNodeId = !isFixed;
    for (const InvariantNodeId& defInvNodeId : _varNodes[i].definingNodes()) {
      if (isFirstInvNodeId) {
        isFirstInvNodeId = false;
      } else {
        replacedDefiningNodes.emplace_back(defInvNodeId);
      }
    }

    std::vector<VarNodeId> splitNodes;
    splitNodes.reserve(replacedDefiningNodes.size() + 1);
    splitNodes.emplace_back(_varNodes[i].varNodeId());

    for (const auto& invNodeId : replacedDefiningNodes) {
      VarNode& newVarNode =
          _varNodes[i].isIntVar()
              ? _varNodes.emplace_back(nextVarNodeId(), true,
                                       SearchDomain(_varNodes[i].lowerBound(),
                                                    _varNodes[i].upperBound()),
                                       VarNode::DomainType::NONE)
              : _varNodes.emplace_back(nextVarNodeId(), false,
                                       VarNode::DomainType::NONE);
      splitNodes.emplace_back(newVarNode.varNodeId());
      invariantNode(invNodeId).replaceDefinedVar(_varNodes[i], newVarNode);
      if (isFixed) {
        if (!_varNodes[i].isIntVar()) {
          newVarNode.fixToValue(_varNodes[i].inDomain(bool{true}));
        }
        assert(newVarNode.isFixed());
        newVarNode.setDomainType(VarNode::DomainType::FIXED);
      }
    }

    if (!isFixed) {
      if (_varNodes[i].isIntVar()) {
        addInvariantNode(std::make_unique<IntAllEqualNode>(
            std::move(splitNodes), true, true));
      } else {
        addInvariantNode(std::make_unique<BoolAllEqualNode>(
            std::move(splitNodes), true, true));
      }
    }
  }
  assert(_varNodes.size() == newSize);
}

std::pair<VarNodeId, InvariantNodeId> InvariantGraph::findPivotInCycle(
    const std::vector<VarNodeId>& cycle) {
  assert(!cycle.empty());
  VarNodeId pivot = cycle.front();
  InvariantNodeId listeningInvariant =
      varNode(cycle[1 % cycle.size()]).outputOf();

  size_t minDomainSize =
      !varNode(pivot).isIntVar() ? 2 : (varNode(pivot).constDomain().size());
  for (size_t i = 1; i < cycle.size() && minDomainSize > 2; ++i) {
    VarNode& node = varNode(cycle[i]);
    if (!node.isIntVar() || node.constDomain().size() < minDomainSize) {
      minDomainSize = !node.isIntVar() ? 2 : node.constDomain().size();
      pivot = node.varNodeId();
      listeningInvariant = varNode(cycle[(i + 1) % cycle.size()]).outputOf();
      assert(std::any_of(
          invariantNode(listeningInvariant).staticInputVarNodeIds().begin(),
          invariantNode(listeningInvariant).staticInputVarNodeIds().end(),
          [&](VarNodeId input) { return input == pivot; }));
    }
  }
  assert(pivot != NULL_NODE_ID);
  assert(minDomainSize > 0);
  return std::pair<VarNodeId, InvariantNodeId>{pivot, listeningInvariant};
}

VarNodeId InvariantGraph::breakCycle(const std::vector<VarNodeId>& cycle) {
  const auto [pivot, listeningInvariant] = findPivotInCycle(cycle);

  assert(pivot != NULL_NODE_ID);
  assert(!varNode(pivot).isFixed());

  VarNode& newInputNode =
      (varNode(pivot).isIntVar()
           ? _varNodes.emplace_back(nextVarNodeId(), true,
                                    SearchDomain(varNode(pivot).lowerBound(),
                                                 varNode(pivot).upperBound()),
                                    VarNode::DomainType::NONE)
           : _varNodes.emplace_back(nextVarNodeId(), false,
                                    VarNode::DomainType::NONE));

  invariantNode(listeningInvariant)
      .replaceStaticInputVarNode(varNode(pivot), newInputNode);
  if (varNode(pivot).isIntVar()) {
    addInvariantNode(
        std::make_unique<IntAllEqualNode>(pivot, newInputNode.varNodeId()));
  } else {
    addInvariantNode(
        std::make_unique<BoolAllEqualNode>(pivot, newInputNode.varNodeId()));
  }
  root().addSearchVarNode(newInputNode);
  return newInputNode.varNodeId();
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
    if (implicitConstraint->state() == InvariantNodeState::ACTIVE) {
      for (const auto& searchVarNodeId :
           implicitConstraint->outputVarNodeIds()) {
        searchVars.emplace(searchVarNodeId);
      }
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
    if (invariantNode(invNodeId).state() != InvariantNodeState::ACTIVE) {
      continue;
    }

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
      varNode(varNodeId).setVarId(solver.makeIntVar(
          constant.value(), constant.value(), constant.value()));
    }
  }

  for (const auto& [constant, varNodeId] : _intVarNodeIndices) {
    if (varId(varNodeId) == propagation::NULL_ID &&
        !varNode(varNodeId).inputTo().empty()) {
      assert(varNode(varNodeId).constantValue().has_value() &&
             varNode(varNodeId).constantValue().value() == constant);
      varNode(varNodeId).setVarId(
          solver.makeIntVar(constant, constant, constant));
    }
  }

  assert(std::all_of(
      _invariantNodes.begin(), _invariantNodes.end(), [&](const auto& invNode) {
        return invNode->state() == InvariantNodeState::SUBSUMED ||
               visitedInvNodes.contains(invNode->id());
      }));
}

void InvariantGraph::createImplicitConstraints(
    propagation::SolverBase& solver) {
  for (auto& implicitConstraintNode : _implicitConstraintNodes) {
    if (implicitConstraintNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::all_of(implicitConstraintNode->outputVarNodeIds().begin(),
                         implicitConstraintNode->outputVarNodeIds().end(),
                         [&](VarNodeId varNodeId) {
                           return varId(varNodeId) != propagation::NULL_ID;
                         }));
      implicitConstraintNode->registerNode(*this, solver);
    }
  }
}

void InvariantGraph::createInvariants(propagation::SolverBase& solver) {
  for (auto& invariantNode : _invariantNodes) {
    if (invariantNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::all_of(invariantNode->outputVarNodeIds().begin(),
                         invariantNode->outputVarNodeIds().end(),
                         [&](VarNodeId varNodeId) {
                           return varId(varNodeId) != propagation::NULL_ID;
                         }));
      invariantNode->registerNode(*this, solver);
    }
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

  for (auto& vNode : _varNodes) {
    if (vNode.varId() != propagation::NULL_ID) {
      const propagation::VarId violationId = vNode.postDomainConstraint(solver);
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
  replaceInvariantNodes();
  populateRootNode();
  splitMultiDefinedVars();
  breakCycles();
  solver.open();
  createVars(solver);
  createImplicitConstraints(solver);
  createInvariants(solver);
  solver.computeBounds();
  _totalViolationVarId = createViolations(solver);
  if (_totalViolationVarId == propagation::NULL_ID ||
      _objectiveVarNodeId == NULL_NODE_ID) {
    auto& trueBoolVarNode = varNode(varNodeId(true));
    if (trueBoolVarNode.varId() == propagation::NULL_ID) {
      trueBoolVarNode.setVarId(solver.makeIntVar(0, 0, 0));
    }
  }
  if (_totalViolationVarId == propagation::NULL_ID) {
    // We use the true Boolean fixed variable (any fixed variable will do):
    _totalViolationVarId = varId(varNodeId(true));
  }
  if (_objectiveVarNodeId == NULL_NODE_ID) {
    // We use the true Boolean fixed variable (any fixed variable will do):
    _objectiveVarNodeId = varNodeId(true);
  }
  assert(_objectiveVarNodeId != NULL_NODE_ID);
  assert(varId(_objectiveVarNodeId) != propagation::NULL_ID);
  solver.close();
}

}  // namespace atlantis::invariantgraph
