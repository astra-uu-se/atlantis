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
    if (vNode.definingNodes().empty() &&
        (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty()) &&
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

InvariantGraph::Edge InvariantGraph::findPivotInCycle(
    const std::vector<Edge>& cycle) {
// Each edge (v, i) is from a variable v to an invariant that v is a static
// input variable to
#ifndef NDEBUG
  for (size_t i = 0; i < cycle.size(); ++i) {
    const VarNode& input =
        varNode(cycle.at((i + cycle.size() - 1) % cycle.size()).varNodeId);
    InvariantNode& invNode = invariantNode(cycle.at(i).invariantNodeId);
    const VarNode& output = varNode(cycle.at(i).varNodeId);

    assert(std::any_of(
        invNode.staticInputVarNodeIds().begin(),
        invNode.staticInputVarNodeIds().end(),
        [&](const VarNodeId& vId) { return vId == input.varNodeId(); }));

    assert(std::any_of(
        input.staticInputTo().begin(), input.staticInputTo().end(),
        [&](const InvariantNodeId& invId) { return invId == invNode.id(); }));

    assert(std::any_of(
        output.definingNodes().begin(), output.definingNodes().end(),
        [&](const InvariantNodeId& invId) { return invId == invNode.id(); }));

    assert(std::any_of(
        invNode.outputVarNodeIds().begin(), invNode.outputVarNodeIds().end(),
        [&](const VarNodeId& vId) { return vId == output.varNodeId(); }));
  }
#endif
  assert(!cycle.empty());
  VarNodeId pivot = cycle.front().varNodeId;
  InvariantNodeId listeningInvariant = cycle.back().invariantNodeId;

  assert(std::any_of(
      invariantNode(listeningInvariant).staticInputVarNodeIds().begin(),
      invariantNode(listeningInvariant).staticInputVarNodeIds().end(),
      [&](VarNodeId input) { return input == pivot; }));

  assert(std::any_of(
      varNodeConst(pivot).staticInputTo().begin(),
      varNodeConst(pivot).staticInputTo().end(),
      [&](InvariantNodeId inv) { return inv == listeningInvariant; }));

  size_t minDomainSize = varNodeConst(pivot).isIntVar()
                             ? varNodeConst(pivot).constDomain().size()
                             : 2;

  for (size_t i = 1; i < cycle.size() && minDomainSize > 2; ++i) {
    const VarNode& vNode = varNodeConst(cycle[i].varNodeId);
    if (!vNode.isIntVar() || vNode.constDomain().size() < minDomainSize) {
      minDomainSize = !vNode.isIntVar() ? 2 : vNode.constDomain().size();
      pivot = vNode.varNodeId();
      listeningInvariant = cycle.at(i - 1).invariantNodeId;
      assert(std::any_of(
          invariantNode(listeningInvariant).staticInputVarNodeIds().begin(),
          invariantNode(listeningInvariant).staticInputVarNodeIds().end(),
          [&](VarNodeId input) { return input == pivot; }));
    }
  }
  assert(pivot != NULL_NODE_ID);
  assert(minDomainSize > 0);
  return Edge{listeningInvariant, pivot};
}

VarNodeId InvariantGraph::breakCycle(const std::vector<Edge>& cycle) {
  // Each edge (v, i) is from a variable v to an invariant that v is a static
  // input variable to
  const auto [listeningInvariant, pivot] = findPivotInCycle(cycle);

  assert(pivot != NULL_NODE_ID);
  assert(listeningInvariant != NULL_NODE_ID);

  // Dont create a VarNode& reference to pivot, since the _varNodes vector is
  // modified, this reference could be invalidated!
  assert(!varNodeConst(pivot).isFixed());

  VarNode& newInputNode =
      _varNodes.emplace_back(nextVarNodeId(), varNodeConst(pivot).isIntVar(),
                             SearchDomain(varNodeConst(pivot).lowerBound(),
                                          varNodeConst(pivot).upperBound()),
                             VarNode::DomainType::NONE);

  invariantNode(listeningInvariant)
      .replaceStaticInputVarNode(varNode(pivot), newInputNode);
  if (varNodeConst(pivot).isIntVar()) {
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
    std::unordered_map<VarNodeId, Edge, VarNodeIdHash>& path) {
  // The path here is a map (x, (i, y)), where the variable x is a static input
  // to invariant i and y is an output variable of i.
  // Note therefore that each Edge is from an invariant i to a variable y that i
  // defines.
  if (visitedGlobal.contains(varNodeId)) {
    return VarNodeId(NULL_NODE_ID);
  }

  visitedLocal.emplace(varNodeId);
  // iterate over all invariants that the variable is a static input to:
  for (const InvariantNodeId& listeningInvNodeId :
       varNode(varNodeId).staticInputTo()) {
    // iterate over all variables that the invariant defines:
    for (const VarNodeId& definedVarId :
         invariantNode(listeningInvNodeId).outputVarNodeIds()) {
      if (!visitedGlobal.contains(definedVarId) &&
          !visitedLocal.contains(definedVarId)) {
        // we have not visited this variable yet, add it to the path:
        // each edge is from a defining invariant i to a variable i defines:
        path.emplace(varNodeId, Edge{listeningInvNodeId, definedVarId});
        const VarNodeId cycleRoot =
            findCycleUtil(definedVarId, visitedGlobal, visitedLocal, path);
        if (cycleRoot != NULL_NODE_ID) {
          return cycleRoot;
        }
      } else if (path.contains(definedVarId)) {
        // this is the first variable that is in the cycle
        // add the last edge to the path (each edge is from a defining invariant
        // i to a variable i defines):
        path.emplace(varNodeId, Edge{listeningInvNodeId, definedVarId});
        // return the root node of the cycle:
        return definedVarId;
      }
    }
  }
  // this variable is not part of a cycle, remove it from the path and return
  // null:
  path.erase(varNodeId);
  return VarNodeId(NULL_NODE_ID);
}

std::vector<VarNodeId> InvariantGraph::breakCycles(
    VarNodeId node,
    std::unordered_set<VarNodeId, VarNodeIdHash>& visitedGlobal) {
  std::unordered_map<VarNodeId, Edge, VarNodeIdHash> path;
  // The path here is a map (x, (i, y)), where the variable x is a static input
  // to invariant i and y is an output variable of i.
  // Note therefore that each Edge is from an invariant i to a variable y that i
  // defines.
  std::unordered_set<VarNodeId, VarNodeIdHash> visitedLocal;
  VarNodeId cycleRoot;
  std::vector<VarNodeId> newSearchNodes;
  do {
    path.clear();
    visitedLocal.clear();
    cycleRoot = findCycleUtil(node, visitedGlobal, visitedLocal, path);
    assert(std::all_of(path.begin(), path.end(), [&](const auto& pair) {
      const VarNode& inputVar = varNodeConst(pair.first);
      InvariantNode& inv = invariantNode(pair.second.invariantNodeId);
      const VarNode& outputVar = varNodeConst(pair.second.varNodeId);

      const bool isInput = std::any_of(inputVar.staticInputTo().begin(),
                                       inputVar.staticInputTo().end(),
                                       [&](const InvariantNodeId& invNodeId) {
                                         return invNodeId == inv.id();
                                       });
      const bool isOutput = std::any_of(outputVar.definingNodes().begin(),
                                        outputVar.definingNodes().end(),
                                        [&](const InvariantNodeId& invNodeId) {
                                          return invNodeId == inv.id();
                                        });
      const bool hasInput = std::any_of(
          inv.staticInputVarNodeIds().begin(),
          inv.staticInputVarNodeIds().end(), [&](const VarNodeId& varNodeId) {
            return varNodeId == inputVar.varNodeId();
          });
      const bool hasOutput = std::any_of(
          inv.outputVarNodeIds().begin(), inv.outputVarNodeIds().end(),
          [&](const VarNodeId& varNodeId) {
            return varNodeId == outputVar.varNodeId();
          });
      assert(isInput);
      assert(isOutput);
      assert(hasInput);
      assert(hasOutput);

      return isInput && isOutput && hasInput && hasOutput;
    }));

    if (cycleRoot != NULL_NODE_ID) {
      // Each edge (i, v) in cycle is from a defining invariant i to the
      // variable v that i defines:
      std::vector<Edge> cycle;
      VarNodeId cur = cycleRoot;
      while (cycle.empty() || cur != cycleRoot) {
        assert(path.contains(cur));
        cycle.emplace_back(Edge{path.at(cur)});
        cur = path.at(cur).varNodeId;
      }
      assert(path.contains(cur));
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

  for (const auto& varNodeId : _boolVarNodeIndices) {
    VarNode& vNode = varNode(varNodeId);
    if (vNode.varId() == propagation::NULL_ID &&
        (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty())) {
      auto constant = vNode.constantValue();
      assert(constant.has_value());
      vNode.setVarId(solver.makeIntVar(constant.value(), constant.value(),
                                       constant.value()));
    }
  }

  for (const auto& [constant, varNodeId] : _intVarNodeIndices) {
    VarNode& vNode = varNode(varNodeId);
    if (vNode.varId() == propagation::NULL_ID &&
        (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty())) {
      assert(vNode.constantValue().has_value() &&
             vNode.constantValue().value() == constant);
      vNode.setVarId(solver.makeIntVar(constant, constant, constant));
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
    if (varNode.definingNodes().empty() &&
        (!varNode.staticInputTo().empty() ||
         !varNode.dynamicInputTo().empty())) {
      assert(varNode.isFixed());
      if (varNode.varId() == propagation::NULL_ID) {
        varNode.setVarId(solver.makeIntVar(
            varNode.lowerBound(), varNode.upperBound(), varNode.lowerBound()));
        for (const auto& invNodeId : varNode.staticInputTo()) {
          if (!visitedInvNodes.contains(invNodeId)) {
            visitedInvNodes.emplace(invNodeId);
            unregisteredInvNodes.emplace(invNodeId);
          }
        }
        for (const auto& invNodeId : varNode.dynamicInputTo()) {
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

      outputVarNodeIds.emplace(outputVarNodeId);
      assert(vn.varId() != propagation::NULL_ID);
      for (const auto& nextVarDefNode : vn.staticInputTo()) {
        if (!visitedInvNodes.contains(nextVarDefNode)) {
          visitedInvNodes.emplace(nextVarDefNode);
          unregisteredInvNodes.emplace(nextVarDefNode);
        }
      }
      for (const auto& nextVarDefNode : vn.dynamicInputTo()) {
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
  sanity(false);
  replaceInvariantNodes();
  sanity(false);
  populateRootNode();
  sanity(false);
  splitMultiDefinedVars();
  sanity(true);
  breakCycles();
  sanity(true);
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
}

void InvariantGraph::close(propagation::SolverBase& solver) { solver.close(); }

void InvariantGraph::sanity(bool oneDefInv) {
#ifndef NDEBUG
  for (const VarNode& vNode : _varNodes) {
    for (const InvariantNodeId& invNodeId : vNode.definingNodes()) {
      InvariantNode& invNode = invariantNode(invNodeId);
      assert(std::any_of(
          invNode.outputVarNodeIds().begin(), invNode.outputVarNodeIds().end(),
          [&](const VarNodeId& vId) { return vId == vNode.varNodeId(); }));
    }
    for (const InvariantNodeId& invNodeId : vNode.staticInputTo()) {
      InvariantNode& invNode = invariantNode(invNodeId);
      assert(std::any_of(
          invNode.staticInputVarNodeIds().begin(),
          invNode.staticInputVarNodeIds().end(),
          [&](const VarNodeId& vId) { return vId == vNode.varNodeId(); }));
    }
    for (const InvariantNodeId& invNodeId : vNode.dynamicInputTo()) {
      InvariantNode& invNode = invariantNode(invNodeId);
      assert(std::any_of(
          invNode.dynamicInputVarNodeIds().begin(),
          invNode.dynamicInputVarNodeIds().end(),
          [&](const VarNodeId& vId) { return vId == vNode.varNodeId(); }));
    }
    if (oneDefInv) {
      assert(vNode.definingNodes().size() <= 1);
    }
  }
  for (const auto& implNode : _implicitConstraintNodes) {
    for (const VarNodeId& vId : implNode->outputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::any_of(vNode.definingNodes().begin(),
                         vNode.definingNodes().end(),
                         [&](const InvariantNodeId& invId) {
                           return invId == implNode->id();
                         }));
    }
    assert(implNode->staticInputVarNodeIds().empty());
    assert(implNode->dynamicInputVarNodeIds().empty());
  }
  for (const auto& invNode : _invariantNodes) {
    for (const VarNodeId& vId : invNode->outputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::any_of(vNode.definingNodes().begin(),
                         vNode.definingNodes().end(),
                         [&](const InvariantNodeId& invId) {
                           return invId == invNode->id();
                         }));
    }
    for (const VarNodeId& vId : invNode->staticInputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::any_of(vNode.staticInputTo().begin(),
                         vNode.staticInputTo().end(),
                         [&](const InvariantNodeId& invId) {
                           return invId == invNode->id();
                         }));
    }
    for (const VarNodeId& vId : invNode->dynamicInputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::any_of(vNode.dynamicInputTo().begin(),
                         vNode.dynamicInputTo().end(),
                         [&](const InvariantNodeId& invId) {
                           return invId == invNode->id();
                         }));
    }
  }
#endif
}

}  // namespace atlantis::invariantgraph
