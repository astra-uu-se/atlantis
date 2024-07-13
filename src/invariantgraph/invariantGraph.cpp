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

void InvariantGraph::replaceFixedVars() {
  // replace all fixed input variables:
  for (VarNode& varNode : _varNodes) {
    if (varNode.definingNodes().empty() && !varNode.inputTo().empty() &&
        varNode.isFixed()) {
      const VarNodeId vId = varNode.varNodeId();
      if (varNode.isIntVar()) {
        if (!_intVarNodeIndices.contains(varNode.lowerBound())) {
          _intVarNodeIndices.emplace(varNode.lowerBound(), vId);
        } else if (_intVarNodeIndices.at(varNode.lowerBound()) != vId) {
          replaceVarNode(vId, _intVarNodeIndices.at(varNode.lowerBound()));
        }
      } else {
        const bool val = varNode.inDomain(bool{true});
        const size_t index = val ? 1 : 0;
        assert(varNodeConst(_boolVarNodeIndices[index]).inDomain(val));
        if (_boolVarNodeIndices[index] != vId) {
          replaceVarNode(vId, _boolVarNodeIndices[index]);
        }
      }
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
    const InvariantNodeId invNodeId = *(oldNode.definingNodes().begin());
    invariantNode(invNodeId).replaceDefinedVar(oldNode, newNode);
    assert(!oldNode.definingNodes().contains(invNodeId));
    assert(newNode.definingNodes().contains(invNodeId));
    assert(std::none_of(invariantNode(invNodeId).outputVarNodeIds().begin(),
                        invariantNode(invNodeId).outputVarNodeIds().end(),
                        [&](const VarNodeId& id) { return id == oldNodeId; }));
    assert(std::any_of(invariantNode(invNodeId).outputVarNodeIds().begin(),
                       invariantNode(invNodeId).outputVarNodeIds().end(),
                       [&](const VarNodeId& id) { return id == newNodeId; }));
  }
  assert(oldNode.definingNodes().empty());

  while (!oldNode.staticInputTo().empty()) {
    const InvariantNodeId invNodeId = oldNode.staticInputTo().front();
    invariantNode(oldNode.staticInputTo().front())
        .replaceStaticInputVarNode(oldNode, newNode);
    assert(std::none_of(
        oldNode.staticInputTo().begin(), oldNode.staticInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(std::any_of(
        newNode.staticInputTo().begin(), newNode.staticInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(
        std::none_of(invariantNode(invNodeId).staticInputVarNodeIds().begin(),
                     invariantNode(invNodeId).staticInputVarNodeIds().end(),
                     [&](const VarNodeId& id) { return id == oldNodeId; }));
    assert(std::any_of(invariantNode(invNodeId).staticInputVarNodeIds().begin(),
                       invariantNode(invNodeId).staticInputVarNodeIds().end(),
                       [&](const VarNodeId& id) { return id == newNodeId; }));
  }
  assert(oldNode.staticInputTo().empty());

  while (!oldNode.dynamicInputTo().empty()) {
    const InvariantNodeId invNodeId = oldNode.dynamicInputTo().front();
    invariantNode(oldNode.dynamicInputTo().front())
        .replaceStaticInputVarNode(oldNode, newNode);
    assert(std::none_of(
        oldNode.dynamicInputTo().begin(), oldNode.dynamicInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(std::any_of(
        newNode.dynamicInputTo().begin(), newNode.dynamicInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(
        std::none_of(invariantNode(invNodeId).dynamicInputVarNodeIds().begin(),
                     invariantNode(invNodeId).dynamicInputVarNodeIds().end(),
                     [&](const VarNodeId& id) { return id == oldNodeId; }));
    assert(
        std::any_of(invariantNode(invNodeId).dynamicInputVarNodeIds().begin(),
                    invariantNode(invNodeId).dynamicInputVarNodeIds().end(),
                    [&](const VarNodeId& id) { return id == newNodeId; }));
  }
  assert(oldNode.dynamicInputTo().empty());
  assert(oldNode.inputTo().empty());

  if (oldNode.isFixed()) {
    if (oldNode.isIntVar() &&
        _intVarNodeIndices.contains(oldNode.lowerBound()) &&
        _intVarNodeIndices.at(oldNode.lowerBound()) == oldNodeId) {
      _intVarNodeIndices.erase(oldNode.lowerBound());
      _intVarNodeIndices.emplace(oldNode.lowerBound(), newNodeId);
    } else {
      const bool val = oldNode.inDomain(bool{true});
      const size_t index = val ? 1 : 0;
      assert(varNodeConst(_boolVarNodeIndices.at(index)).inDomain(val));
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
      assert(vNode.definingNodes().contains(root().id()));
    }
  }
}

void InvariantGraph::splitMultiDefinedVars() {
  // DO NOT empace to _varNodes while doing this kind of iteration!

  size_t newSize = 0;
  for (const auto& vNode : _varNodes) {
    newSize += std::max<size_t>(
        1, vNode.definingNodes().size() + (vNode.isFixed() ? 1 : 0));
  }
  assert(_varNodes.size() <= newSize);
  _varNodes.reserve(newSize);

  const size_t end = _varNodes.size();

  for (size_t i = 0; i < end; i++) {
    const bool isFixed = _varNodes[i].isFixed();
    const size_t numSplitNodes =
        _varNodes[i].definingNodes().size() + (isFixed ? 1 : 0);
    if (numSplitNodes <= 1) {
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
    splitNodes.reserve(isFixed ? 0 : numSplitNodes);
    if (!isFixed) {
      splitNodes.emplace_back(_varNodes[i].varNodeId());
    }

    for (const auto& invNodeId : replacedDefiningNodes) {
      VarNode& splitVarNode = _varNodes.emplace_back(
          nextVarNodeId(), _varNodes[i].isIntVar(),
          SearchDomain{_varNodes[i].constDomain()},
          isFixed ? VarNode::DomainType::FIXED : VarNode::DomainType::NONE);

      invariantNode(invNodeId).replaceDefinedVar(_varNodes[i], splitVarNode);

      if (!isFixed) {
        splitNodes.emplace_back(splitVarNode.varNodeId());
      }
    }

    assert(splitNodes.empty() == isFixed);

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
    const std::vector<std::pair<VarNodeId, InvariantNodeId>>& cycle) {
  assert(!cycle.empty());
  VarNodeId pivot = cycle.front().first;
  InvariantNodeId listeningInvNodeId = cycle.front().second;

  size_t minDomainSize =
      !varNode(pivot).isIntVar() ? 2 : (varNode(pivot).constDomain().size());

  for (size_t i = 1; i < cycle.size() && minDomainSize > 2; ++i) {
    const VarNode& node = varNodeConst(cycle.at(i).first);

    if (!node.isIntVar() || node.constDomain().size() < minDomainSize) {
      minDomainSize = !node.isIntVar() ? 2 : node.constDomain().size();
      pivot = node.varNodeId();
      listeningInvNodeId = cycle.at(i).second;
      assert(std::any_of(
          invariantNode(listeningInvNodeId).staticInputVarNodeIds().begin(),
          invariantNode(listeningInvNodeId).staticInputVarNodeIds().end(),
          [&](const VarNodeId& input) { return input == pivot; }));
    }
  }
  assert(pivot != NULL_NODE_ID);
  assert(minDomainSize > 0);
  return std::pair<VarNodeId, InvariantNodeId>{pivot, listeningInvNodeId};
}

VarNodeId InvariantGraph::breakCycle(
    const std::vector<std::pair<VarNodeId, InvariantNodeId>>& cycle) {
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
    std::unordered_map<VarNodeId, std::pair<InvariantNodeId, VarNodeId>,
                       VarNodeIdHash>& path) {
  if (visitedGlobal.contains(varNodeId)) {
    return VarNodeId(NULL_NODE_ID);
  }

  visitedLocal.emplace(varNodeId);
  for (const InvariantNodeId& listeningInvNodeId :
       varNode(varNodeId).staticInputTo()) {
    assert(std::any_of(
        invariantNode(listeningInvNodeId).staticInputVarNodeIds().begin(),
        invariantNode(listeningInvNodeId).staticInputVarNodeIds().end(),
        [&](const VarNodeId& input) { return input == varNodeId; }));

    for (const VarNodeId& definedVarId :
         invariantNode(listeningInvNodeId).outputVarNodeIds()) {
      assert(definedVarId != NULL_NODE_ID);
      assert(varNodeConst(definedVarId).definingNodes().size() == 1 &&
             varNodeConst(definedVarId).outputOf() == listeningInvNodeId);
      if (!visitedGlobal.contains(definedVarId) &&
          !visitedLocal.contains(definedVarId)) {
        path.emplace(varNodeId, std::pair<InvariantNodeId, VarNodeId>{
                                    listeningInvNodeId, definedVarId});
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
  std::unordered_map<VarNodeId, std::pair<InvariantNodeId, VarNodeId>,
                     VarNodeIdHash>
      path;
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedLocal;
  VarNodeId cycleRoot;
  std::vector<VarNodeId> newSearchNodes;
  do {
    path.clear();
    visitedLocal.clear();
    cycleRoot = findCycleUtil(node, visitedGlobal, visitedLocal, path);
    if (cycleRoot != NULL_NODE_ID) {
      std::vector<std::pair<VarNodeId, InvariantNodeId>> cycle;
      VarNodeId curVar = cycleRoot;
      for (; path.contains(curVar); curVar = path.at(curVar).second) {
        assert(cycle.empty() || curVar != cycleRoot);

        assert(varNodeConst(path.at(curVar).second).outputOf() ==
               path.at(curVar).first);

        cycle.emplace_back(std::pair<VarNodeId, InvariantNodeId>{
            curVar, path.at(curVar).first});

        assert(std::any_of(varNodeConst(curVar).staticInputTo().begin(),
                           varNodeConst(curVar).staticInputTo().end(),
                           [&](const InvariantNodeId& invNodeId) {
                             return invNodeId == cycle.back().second;
                           }));
      }

      assert(std::any_of(varNodeConst(curVar).staticInputTo().begin(),
                         varNodeConst(curVar).staticInputTo().end(),
                         [&](const InvariantNodeId& invNodeId) {
                           return invNodeId ==
                                  varNodeConst(cycleRoot).outputOf();
                         }));

      cycle.emplace_back(std::pair<VarNodeId, InvariantNodeId>{
          curVar, varNodeConst(cycleRoot).outputOf()});

      assert(std::any_of(cycle.begin(), cycle.end(),
                         [&](const std::pair<VarNodeId, InvariantNodeId>& p) {
                           return p.first == cycleRoot;
                         }));
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

  for (const VarNode& vNode : _varNodes) {
    if (vNode.definingNodes().empty() && !vNode.inputTo().empty()) {
      assert(vNode.isFixed());
      assert((vNode.isIntVar() &&
              varNodeId(vNode.lowerBound()) == vNode.varNodeId()) ||
             (!vNode.isIntVar() &&
              varNodeId(vNode.inDomain(bool{true})) == vNode.varNodeId()));
      searchNodeIds.emplace(vNode.varNodeId());
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

  for (VarNode& varNode : _varNodes) {
    if (varNode.definingNodes().empty() && !varNode.inputTo().empty()) {
      assert(varNode.isFixed());
      if (varNode.varId() == propagation::NULL_ID) {
        varNode.setVarId(solver.makeIntVar(
            varNode.lowerBound(), varNode.upperBound(), varNode.lowerBound()));
        for (const auto& invNodeId : varNode.inputTo()) {
          if (!visitedInvNodes.contains(invNodeId)) {
            visitedInvNodes.emplace(invNodeId);
            unregisteredInvNodes.emplace(invNodeId);
          }
        }
      }
    }
  }

  std::unordered_set<VarNodeId, VarNodeIdHash> outputVarNodeIds;

  while (!unregisteredInvNodes.empty()) {
    const auto invNodeId = unregisteredInvNodes.front();
    unregisteredInvNodes.pop();
    assert(visitedInvNodes.contains(invNodeId));
    auto& invNode = invariantNode(invNodeId);
    // If the invNodeId only defines a single variable, then it is a view:
    assert(!invNode.dynamicInputVarNodeIds().empty() ||
           invNode.staticInputVarNodeIds().size() != 1 ||
           varId(invNode.staticInputVarNodeIds().front()) !=
               propagation::NULL_ID);
    assert(std::all_of(invNode.outputVarNodeIds().begin(),
                       invNode.outputVarNodeIds().end(),
                       [&](VarNodeId outputVarNodeId) {
                         return varId(outputVarNodeId) == propagation::NULL_ID;
                       }));
    if (invNode.state() != InvariantNodeState::ACTIVE) {
      continue;
    }

    invNode.registerOutputVars(*this, solver);
    for (const auto& outputVarNodeId : invNode.outputVarNodeIds()) {
      assert(outputVarNodeId != NULL_NODE_ID);
      const auto& vn = varNode(outputVarNodeId);
      assert(std::any_of(
          vn.definingNodes().begin(), vn.definingNodes().end(),
          [&](InvariantNodeId defNodeId) { return defNodeId == invNodeId; }));

      assert(vn.varId() != propagation::NULL_ID);
      for (const auto& nextVarDefNode : vn.inputTo()) {
        if (!visitedInvNodes.contains(nextVarDefNode)) {
          visitedInvNodes.emplace(nextVarDefNode);
          unregisteredInvNodes.emplace(nextVarDefNode);
        }
      }
    }
  }

#ifndef NDEBUG
  for (const auto& invNode : _invariantNodes) {
    if (invNode->state() != InvariantNodeState::SUBSUMED) {
      assert(visitedInvNodes.contains(invNode->id()));
    }
  }
#endif
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
  replaceFixedVars();
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
  if (varId(_objectiveVarNodeId) == propagation::NULL_ID) {
    varNode(_objectiveVarNodeId).setVarId(solver.makeIntVar(1, 1, 1));
  }
  assert(_objectiveVarNodeId != NULL_NODE_ID);
  assert(varId(_objectiveVarNodeId) != propagation::NULL_ID);
  solver.close();
}

}  // namespace atlantis::invariantgraph
