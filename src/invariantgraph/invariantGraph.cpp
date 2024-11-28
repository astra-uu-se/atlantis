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

InvariantGraph::InvariantGraph(propagation::SolverBase& solver,
                               bool breakDynamicCycles)
    : _solver(solver),
      _varNodes{VarNode{VarNodeId{0}, false, SearchDomain({1})},
                VarNode{VarNodeId{1}, false, SearchDomain({0})}},
      _namedVarNodeIndices(),
      _intVarNodeIndices(),
      _boolVarNodeIndices{VarNodeId{0}, VarNodeId{1}},
      _invariantNodes(),
      _implicitConstraintNodes{},
      _breakDynamicCycles(breakDynamicCycles),
      _totalViolationVarId(propagation::NULL_ID),
      _objectiveVarNodeId{NULL_NODE_ID} {
  addImplicitConstraintNode(std::make_shared<InvariantGraphRoot>(*this));
}

propagation::SolverBase& InvariantGraph::solver() { return _solver; }

const propagation::SolverBase& InvariantGraph::solverConst() const {
  return _solver;
}

VarNodeId InvariantGraph::nextVarNodeId() const {
  return VarNodeId(_varNodes.size());
}

bool InvariantGraph::containsVarNode(const std::string& identifier) const {
  return !_namedVarNodeIndices.empty() &&
         _namedVarNodeIndices.contains(identifier);
}

bool InvariantGraph::containsVarNode(Int i) const {
  return !_intVarNodeIndices.empty() && _intVarNodeIndices.contains(i);
}

bool InvariantGraph::containsVarNode(bool) const { return true; }

VarNodeId InvariantGraph::retrieveBoolVarNode(bool b) {
  assert(varNode(_boolVarNodeIndices.at(0)).inDomain(bool{false}));
  assert(varNode(_boolVarNodeIndices.at(1)).inDomain(bool{true}));
  return _boolVarNodeIndices.at(b ? 1 : 0);
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
  if (!containsVarNode((Int)value)) {
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
  assert(size_t(_namedVarNodeIndices.at(identifier)) < _varNodes.size());
  return _varNodes.at(size_t(_namedVarNodeIndices.at(identifier)));
}

VarNode& InvariantGraph::varNode(VarNodeId id) {
  assert(size_t(id) < _varNodes.size());
  return _varNodes.at(size_t(id));
}

VarNode& InvariantGraph::varNode(Int value) {
  assert(_intVarNodeIndices.contains(value));
  assert(size_t(_intVarNodeIndices.at(value)) < _varNodes.size());
  return _varNodes.at(size_t(_intVarNodeIndices.at(value)));
}

void InvariantGraph::replaceInvariantNodes() {
  size_t invIndex = 0;
  size_t implIndex = 0;
  while (invIndex < _invariantNodes.size() &&
         implIndex < _implicitConstraintNodes.size()) {
    while (invIndex < _invariantNodes.size()) {
      auto& invNode = *_invariantNodes.at(invIndex);
      invNode.updateState();
      if (invNode.state() == InvariantNodeState::SUBSUMED) {
        invNode.deactivate();
      } else {
        if (invNode.canBeReplaced()) {
          if (invNode.replace()) {
            invNode.deactivate();
          }
        } else if (invNode.canBeMadeImplicit()) {
          if (invNode.makeImplicit()) {
            invNode.deactivate();
          }
        }
      }
      ++invIndex;
    }
    while (implIndex < _implicitConstraintNodes.size()) {
      auto& implNode = *_implicitConstraintNodes.at(implIndex);
      implNode.updateState();
      if (implNode.state() == InvariantNodeState::SUBSUMED) {
        implNode.deactivate();
      } else {
        if (implNode.canBeReplaced()) {
          if (implNode.replace()) {
            implNode.deactivate();
          }
        }
      }
      ++implIndex;
    }
  }
}

void InvariantGraph::replaceFixedVars() {
  // replace all fixed input variables:
  for (VarNode& vNode : _varNodes) {
    if (vNode.definingNodes().empty() &&
        (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty()) &&
        vNode.isFixed()) {
      const VarNodeId vId = vNode.varNodeId();
      if (vNode.isIntVar()) {
        if (!_intVarNodeIndices.contains(vNode.lowerBound())) {
          _intVarNodeIndices.emplace(vNode.lowerBound(), vId);
        } else if (_intVarNodeIndices.at(vNode.lowerBound()) != vId) {
          replaceVarNode(vId, _intVarNodeIndices.at(vNode.lowerBound()));
        }
      } else {
        const bool val = vNode.inDomain(bool{true});
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
  assert(size_t(_namedVarNodeIndices.at(identifier)) < _varNodes.size());
  return _varNodes.at(size_t(_namedVarNodeIndices.at(identifier)));
}

const VarNode& InvariantGraph::varNodeConst(VarNodeId id) const {
  assert(size_t(id) < _varNodes.size());
  return _varNodes.at(size_t(id));
}

VarNodeId InvariantGraph::varNodeId(const std::string& identifier) const {
  if (!containsVarNode(identifier)) {
    return VarNodeId{NULL_NODE_ID};
  }
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::varNodeId(bool val) const {
  if (!containsVarNode(val)) {
    return VarNodeId{NULL_NODE_ID};
  }
  return _boolVarNodeIndices.at(val ? 1 : 0);
}

VarNodeId InvariantGraph::varNodeId(Int val) const {
  if (!containsVarNode(val)) {
    return VarNodeId{NULL_NODE_ID};
  }
  return _intVarNodeIndices.at(val);
}

propagation::VarViewId InvariantGraph::varId(
    const std::string& identifier) const {
  return _varNodes.at(size_t(_namedVarNodeIndices.at(identifier))).varId();
}

propagation::VarViewId InvariantGraph::varId(VarNodeId id) const {
  assert(size_t(id) < _varNodes.size());
  return _varNodes.at(size_t(id)).varId();
}

bool InvariantGraph::containsInvariantNode(InvariantNodeId id) const {
  return id.isInvariant() && size_t(id) < _invariantNodes.size();
}

bool InvariantGraph::containsImplicitConstraintNode(InvariantNodeId id) const {
  return id.isImplicitConstraint() &&
         size_t(id) < _implicitConstraintNodes.size();
}

IInvariantNode& InvariantGraph::invariantNode(InvariantNodeId id) {
  if (id.isImplicitConstraint()) {
    assert(containsImplicitConstraintNode(id));
    return implicitConstraintNode(id);
  }
  assert(containsInvariantNode(id));
  return *_invariantNodes.at(size_t(id));
}

IImplicitConstraintNode& InvariantGraph::implicitConstraintNode(
    InvariantNodeId id) {
  assert(containsImplicitConstraintNode(id));
  return *_implicitConstraintNodes.at(size_t(id));
}

InvariantNodeId InvariantGraph::nextInvariantNodeId() const {
  return {_invariantNodes.size(), false};
}

InvariantNodeId InvariantGraph::nextImplicitNodeId() const {
  return {_implicitConstraintNodes.size(), true};
}

InvariantNodeId InvariantGraph::addInvariantNode(
    std::shared_ptr<IInvariantNode>&& node) {
  const InvariantNodeId id = nextInvariantNodeId();
  auto& invNode = _invariantNodes.emplace_back(std::move(node));
  invNode->init(id);
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
    invariantNode(invNodeId).replaceDefinedVar(oldNode.varNodeId(),
                                               newNode.varNodeId());
    assert(!oldNode.definingNodes().contains(invNodeId));
    assert(newNode.definingNodes().contains(invNodeId));
    assert(std::none_of(invariantNode(invNodeId).outputVarNodeIds().begin(),
                        invariantNode(invNodeId).outputVarNodeIds().end(),
                        [&](const VarNodeId id) { return id == oldNodeId; }));
    assert(std::any_of(invariantNode(invNodeId).outputVarNodeIds().begin(),
                       invariantNode(invNodeId).outputVarNodeIds().end(),
                       [&](const VarNodeId id) { return id == newNodeId; }));
  }
  assert(oldNode.definingNodes().empty());

  while (!oldNode.staticInputTo().empty()) {
    const InvariantNodeId invNodeId = oldNode.staticInputTo().front();
    invariantNode(oldNode.staticInputTo().front())
        .replaceStaticInputVarNode(oldNode.varNodeId(), newNode.varNodeId());
    assert(std::none_of(
        oldNode.staticInputTo().begin(), oldNode.staticInputTo().end(),
        [&](const InvariantNodeId id) { return id == invNodeId; }));
    assert(std::any_of(
        newNode.staticInputTo().begin(), newNode.staticInputTo().end(),
        [&](const InvariantNodeId id) { return id == invNodeId; }));
    assert(
        std::none_of(invariantNode(invNodeId).staticInputVarNodeIds().begin(),
                     invariantNode(invNodeId).staticInputVarNodeIds().end(),
                     [&](const VarNodeId id) { return id == oldNodeId; }));
    assert(std::any_of(invariantNode(invNodeId).staticInputVarNodeIds().begin(),
                       invariantNode(invNodeId).staticInputVarNodeIds().end(),
                       [&](const VarNodeId id) { return id == newNodeId; }));
  }
  assert(oldNode.staticInputTo().empty());

  while (!oldNode.dynamicInputTo().empty()) {
    const InvariantNodeId invNodeId = oldNode.dynamicInputTo().front();
    invariantNode(oldNode.dynamicInputTo().front())
        .replaceDynamicInputVarNode(oldNode.varNodeId(), newNode.varNodeId());
    assert(std::none_of(
        oldNode.dynamicInputTo().begin(), oldNode.dynamicInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(std::any_of(
        newNode.dynamicInputTo().begin(), newNode.dynamicInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(
        std::none_of(invariantNode(invNodeId).dynamicInputVarNodeIds().begin(),
                     invariantNode(invNodeId).dynamicInputVarNodeIds().end(),
                     [&](const VarNodeId id) { return id == oldNodeId; }));
    assert(
        std::any_of(invariantNode(invNodeId).dynamicInputVarNodeIds().begin(),
                    invariantNode(invNodeId).dynamicInputVarNodeIds().end(),
                    [&](const VarNodeId id) { return id == newNodeId; }));
  }
  assert(oldNode.dynamicInputTo().empty());
  assert(oldNode.staticInputTo().empty());

  if (oldNode.isFixed()) {
    if (oldNode.isIntVar()) {
      if (_intVarNodeIndices.contains(oldNode.lowerBound()) &&
          _intVarNodeIndices.at(oldNode.lowerBound()) == oldNodeId) {
        _intVarNodeIndices.erase(oldNode.lowerBound());
        _intVarNodeIndices.emplace(oldNode.lowerBound(), newNodeId);
      }
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
    std::shared_ptr<IImplicitConstraintNode>&& node) {
  const InvariantNodeId id = nextImplicitNodeId();
  auto& implNode = _implicitConstraintNodes.emplace_back(std::move(node));
  implNode->init(id);
  return implNode->id();
}

search::neighbourhoods::NeighbourhoodCombinator InvariantGraph::neighbourhood()
    const {
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

propagation::VarViewId InvariantGraph::totalViolationVarId() const {
  return _totalViolationVarId;
}

const VarNode& InvariantGraph::objectiveVarNode() const {
  return varNodeConst(_objectiveVarNodeId);
}

propagation::VarViewId InvariantGraph::objectiveVarId() const {
  return varId(_objectiveVarNodeId);
}

void InvariantGraph::populateRootNode() {
  for (auto& vNode : _varNodes) {
    if (vNode.definingNodes().empty() &&
        (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty() ||
         _objectiveVarNodeId == vNode.varNodeId()) &&
        !vNode.isFixed()) {
      root().addSearchVarNode(vNode.varNodeId());
      assert(root().outputVarNodeIds().back() == vNode.varNodeId());
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

      invariantNode(invNodeId).replaceDefinedVar(_varNodes[i].varNodeId(),
                                                 splitVarNode.varNodeId());

      if (!isFixed) {
        splitNodes.emplace_back(splitVarNode.varNodeId());
      }
    }

    assert(splitNodes.empty() == isFixed);

    if (!isFixed) {
      if (_varNodes[i].isIntVar()) {
        addInvariantNode(std::make_shared<IntAllEqualNode>(
            *this, std::move(splitNodes), true, true));
      } else {
        addInvariantNode(std::make_shared<BoolAllEqualNode>(
            *this, std::move(splitNodes), true, true));
      }
    }
  }
  assert(_varNodes.size() == newSize);
}

InvariantGraphEdge InvariantGraph::findPivotInCycle(
    const std::vector<InvariantGraphEdge>& cycle) {
  // Each edge (i, v) is from invariant i to variable v that i defines
  // For each subsequent edges (i1, v1), (i2, v2) in the cycle (wrapped around)
  // v1 is a static input to i2
  assert(cycle.size() > 1);
#ifndef NDEBUG
  for (size_t i = 0; i < cycle.size(); ++i) {
    const size_t inputIndex = (i + cycle.size() - 1) % cycle.size();
    const VarNode& input = varNode(cycle.at(inputIndex).varNodeId);
    IInvariantNode& invNode = invariantNode(cycle.at(i).invariantNodeId);
    const VarNode& output = varNode(cycle.at(i).varNodeId);

    assert(std::any_of(
               invNode.staticInputVarNodeIds().begin(),
               invNode.staticInputVarNodeIds().end(),
               [&](const VarNodeId vId) { return vId == input.varNodeId(); }) ||
           std::any_of(
               invNode.dynamicInputVarNodeIds().begin(),
               invNode.dynamicInputVarNodeIds().end(),
               [&](const VarNodeId vId) { return vId == input.varNodeId(); }));

    assert(std::any_of(input.staticInputTo().begin(),
                       input.staticInputTo().end(),
                       [&](const InvariantNodeId& invId) {
                         return invId == invNode.id();
                       }) ||
           std::any_of(input.dynamicInputTo().begin(),
                       input.dynamicInputTo().end(),
                       [&](const InvariantNodeId& invId) {
                         return invId == invNode.id();
                       }));

    assert(std::any_of(
        output.definingNodes().begin(), output.definingNodes().end(),
        [&](const InvariantNodeId& invId) { return invId == invNode.id(); }));

    assert(std::any_of(
        invNode.outputVarNodeIds().begin(), invNode.outputVarNodeIds().end(),
        [&](const VarNodeId vId) { return vId == output.varNodeId(); }));
  }
#endif
  VarNodeId pivot = cycle[0].varNodeId;
  InvariantNodeId listeningInvNodeId = cycle[1].invariantNodeId;

  assert(std::any_of(
             invariantNode(listeningInvNodeId).staticInputVarNodeIds().begin(),
             invariantNode(listeningInvNodeId).staticInputVarNodeIds().end(),
             [&](VarNodeId input) { return input == pivot; }) ||
         std::any_of(
             invariantNode(listeningInvNodeId).dynamicInputVarNodeIds().begin(),
             invariantNode(listeningInvNodeId).dynamicInputVarNodeIds().end(),
             [&](VarNodeId input) { return input == pivot; }));

  assert(std::any_of(
             varNodeConst(pivot).staticInputTo().begin(),
             varNodeConst(pivot).staticInputTo().end(),
             [&](InvariantNodeId inv) { return inv == listeningInvNodeId; }) ||
         std::any_of(
             varNodeConst(pivot).dynamicInputTo().begin(),
             varNodeConst(pivot).dynamicInputTo().end(),
             [&](InvariantNodeId inv) { return inv == listeningInvNodeId; }));

  size_t minDomainSize = varNodeConst(pivot).isIntVar()
                             ? varNodeConst(pivot).constDomain().size()
                             : 2;

  for (size_t i = 1; i < cycle.size() && minDomainSize > 2; ++i) {
    const VarNode& vNode = varNodeConst(cycle[i].varNodeId);
    if (!vNode.isIntVar() || vNode.constDomain().size() < minDomainSize) {
      minDomainSize = !vNode.isIntVar() ? 2 : vNode.constDomain().size();
      pivot = vNode.varNodeId();
      const size_t listeningInvIndex = i + 1 % cycle.size();
      listeningInvNodeId = cycle[listeningInvIndex].invariantNodeId;
      assert(
          std::any_of(
              invariantNode(listeningInvNodeId).staticInputVarNodeIds().begin(),
              invariantNode(listeningInvNodeId).staticInputVarNodeIds().end(),
              [&](const VarNodeId input) { return input == pivot; }) ||
          std::any_of(
              invariantNode(listeningInvNodeId)
                  .dynamicInputVarNodeIds()
                  .begin(),
              invariantNode(listeningInvNodeId).dynamicInputVarNodeIds().end(),
              [&](const VarNodeId input) { return input == pivot; }));
    }
  }
  assert(pivot != NULL_NODE_ID);
  assert(minDomainSize > 0);
  return InvariantGraphEdge{listeningInvNodeId, pivot};
}

VarNodeId InvariantGraph::breakCycle(
    const std::vector<InvariantGraphEdge>& cycle) {
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
      .replaceStaticInputVarNode(pivot, newInputNode.varNodeId());
  if (varNodeConst(pivot).isIntVar()) {
    addInvariantNode(std::make_shared<IntAllEqualNode>(
        *this, pivot, newInputNode.varNodeId(), true, true));
  } else {
    addInvariantNode(std::make_shared<BoolAllEqualNode>(
        *this, pivot, newInputNode.varNodeId(), true, true));
  }
  root().addSearchVarNode(newInputNode.varNodeId());
  return newInputNode.varNodeId();
}

std::unordered_set<VarNodeId> InvariantGraph::dynamicVarNodeFrontier(
    const VarNodeId node, const std::unordered_set<VarNodeId>& visitedGlobal) {
  std::unordered_set<VarNodeId> visitedLocal;
  std::unordered_set<VarNodeId> visitedDynamic;
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
    const std::unordered_set<VarNodeId>& visitedGlobal,
    std::unordered_set<VarNodeId>& visitedLocal,
    std::unordered_map<VarNodeId, InvariantGraphEdge>& path) {
  assert(!path.contains(varNodeId));
  // The path here is a map (x, (i, y)), where the variable x is a static input
  // to invariant i and y is an output variable of i.
  // Note therefore that each InvariantGraphEdge is from an invariant i to a
  // variable y that i defines.
  if (visitedGlobal.contains(varNodeId)) {
    assert(!path.contains(varNodeId));
    return VarNodeId{NULL_NODE_ID};
  }

  if (visitedLocal.contains(varNodeId)) {
    // this variable is part of a cycle, return the root node of the cycle:
    assert(path.contains(varNodeId));
    return path.at(varNodeId).varNodeId;
  }

  visitedLocal.emplace(varNodeId);
  // iterate over all invariants that the variable is a static input to:
  // TODO: we now also break dynamic cycles. this should be fixed by having a
  // better implicit constraint/neighbourhood initialisation process when
  // closing the propagation _solver.
  for (size_t i = 0; i < (_breakDynamicCycles ? 2 : 1); ++i) {
    for (const InvariantNodeId& listeningInvNodeId :
         i == 0 ? varNode(varNodeId).staticInputTo()
                : varNode(varNodeId).dynamicInputTo()) {
      // iterate over all variables that the invariant defines:
      for (const VarNodeId definedVarId :
           invariantNode(listeningInvNodeId).outputVarNodeIds()) {
        if (path.contains(definedVarId)) {
          assert(visitedLocal.contains(definedVarId));
          // this is the first variable that is in the cycle
          // add the last edge to the path (each edge is from a defining
          // invariant i to a variable i defines):
          auto iter = path.find(varNodeId);
          if (iter == path.end()) {
            path.emplace(varNodeId,
                         InvariantGraphEdge{listeningInvNodeId, definedVarId});
          } else {
            iter->second.invariantNodeId = listeningInvNodeId;
            iter->second.varNodeId = definedVarId;
          }
          // return the root node of the cycle:
          return definedVarId;
        } else if (!visitedGlobal.contains(definedVarId) &&
                   !visitedLocal.contains(definedVarId)) {
          // we have not visited this variable yet, add it to the path:
          // each edge is from a defining invariant i to a variable i defines:
          auto iter = path.find(varNodeId);
          if (iter == path.end()) {
            path.emplace(varNodeId,
                         InvariantGraphEdge{listeningInvNodeId, definedVarId});
          } else {
            iter->second.invariantNodeId = listeningInvNodeId;
            iter->second.varNodeId = definedVarId;
          }
          const VarNodeId cycleRoot =
              findCycleUtil(definedVarId, visitedGlobal, visitedLocal, path);
          if (cycleRoot != NULL_NODE_ID) {
            return cycleRoot;
          }
        }
      }
    }
  }
  // this variable is not part of a cycle, remove it from the path and return
  // null:
  path.erase(varNodeId);
  return VarNodeId{NULL_NODE_ID};
}

void InvariantGraph::breakSelfCycles() {
  for (size_t i = 0; i < _invariantNodes.size(); ++i) {
    std::unordered_set<VarNodeId> visitedOutputs;
    visitedOutputs.reserve(_invariantNodes.at(i)->outputVarNodeIds().size());
    for (size_t j = 0; j < _invariantNodes.at(i)->outputVarNodeIds().size();
         ++j) {
      const VarNodeId outputVarId =
          _invariantNodes.at(i)->outputVarNodeIds().at(j);
      if (visitedOutputs.contains(outputVarId)) {
        continue;
      }
      bool hasSelfCycle = false;
      for (size_t k = 0; k < (_breakDynamicCycles ? 2 : 1); ++k) {
        for (const auto& inputVarId :
             k == 0 ? _invariantNodes.at(i)->staticInputVarNodeIds()
                    : _invariantNodes.at(i)->dynamicInputVarNodeIds()) {
          if (outputVarId == inputVarId) {
            hasSelfCycle = true;
            break;
          }
        }
        if (hasSelfCycle) {
          break;
        }
      }
      if (hasSelfCycle) {
        const VarNodeId newDefinedVar =
            _varNodes
                .emplace_back(
                    nextVarNodeId(), varNodeConst(outputVarId).isIntVar(),
                    SearchDomain(varNodeConst(outputVarId).lowerBound(),
                                 varNodeConst(outputVarId).upperBound()),
                    VarNode::DomainType::NONE)
                .varNodeId();
        _invariantNodes.at(i)->replaceDefinedVar(outputVarId, newDefinedVar);
        if (varNodeConst(outputVarId).isIntVar()) {
          addInvariantNode(std::make_shared<IntAllEqualNode>(
              *this, outputVarId, newDefinedVar, true, true));
        } else {
          addInvariantNode(std::make_shared<BoolAllEqualNode>(
              *this, outputVarId, newDefinedVar, true, true));
        }
        if (varNodeConst(outputVarId).definingNodes().empty()) {
          root().addSearchVarNode(outputVarId);
        }
        visitedOutputs.emplace(newDefinedVar);
      }
    }
  }
}

std::vector<VarNodeId> InvariantGraph::breakCycles(
    VarNodeId node, std::unordered_set<VarNodeId>& visitedGlobal) {
  std::unordered_map<VarNodeId, InvariantGraphEdge> path;
  // The path here is a map (x, (i, y)), where the variable x is a static input
  // to invariant i and y is an output variable of i.
  // Note therefore that each InvariantGraphEdge is from an invariant i to a
  // variable y that i defines.
  std::unordered_set<VarNodeId> visitedLocal;
  VarNodeId cycleRoot;
  std::vector<VarNodeId> newSearchNodes;
  do {
    path.clear();
    visitedLocal.clear();
    cycleRoot = findCycleUtil(node, visitedGlobal, visitedLocal, path);
    assert(std::all_of(path.begin(), path.end(), [&](const auto& pair) {
      const VarNode& inputVar = varNodeConst(pair.first);
      IInvariantNode& inv = invariantNode(pair.second.invariantNodeId);
      const VarNode& outputVar = varNodeConst(pair.second.varNodeId);

      const bool isStaticInput = std::any_of(
          inputVar.staticInputTo().begin(), inputVar.staticInputTo().end(),
          [&](const InvariantNodeId& invNodeId) {
            return invNodeId == inv.id();
          });
      const bool isDynamicInput =
          _breakDynamicCycles &&
          std::any_of(inputVar.dynamicInputTo().begin(),
                      inputVar.dynamicInputTo().end(),
                      [&](const InvariantNodeId& invNodeId) {
                        return invNodeId == inv.id();
                      });
      const bool isOutput = std::any_of(outputVar.definingNodes().begin(),
                                        outputVar.definingNodes().end(),
                                        [&](const InvariantNodeId& invNodeId) {
                                          return invNodeId == inv.id();
                                        });
      const bool hasStaticInput = std::any_of(
          inv.staticInputVarNodeIds().begin(),
          inv.staticInputVarNodeIds().end(), [&](const VarNodeId varNodeId) {
            return varNodeId == inputVar.varNodeId();
          });
      const bool hasDynamicInput =
          _breakDynamicCycles &&
          std::any_of(inv.dynamicInputVarNodeIds().begin(),
                      inv.dynamicInputVarNodeIds().end(),
                      [&](const VarNodeId varNodeId) {
                        return varNodeId == inputVar.varNodeId();
                      });
      const bool hasOutput = std::any_of(
          inv.outputVarNodeIds().begin(), inv.outputVarNodeIds().end(),
          [&](const VarNodeId varNodeId) {
            return varNodeId == outputVar.varNodeId();
          });
      assert(isStaticInput || isDynamicInput);
      assert(isOutput);
      assert(hasStaticInput || hasDynamicInput);
      assert(hasOutput);

      assert(isStaticInput == hasStaticInput);
      assert(isDynamicInput == hasDynamicInput);

      return (isStaticInput || isDynamicInput) && isOutput &&
             (hasStaticInput || hasDynamicInput) && hasOutput;
    }));

    if (cycleRoot != NULL_NODE_ID) {
      // Each edge (i, v) in cycle is from a defining invariant i to the
      // variable v that i defines:
      std::vector<InvariantGraphEdge> cycle;
      VarNodeId cur = cycleRoot;
      while (cycle.empty() || cur != cycleRoot) {
        assert(path.contains(cur));
        cycle.emplace_back(InvariantGraphEdge{path.at(cur)});
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
  std::unordered_set<VarNodeId> visitedGlobal;
  std::queue<VarNodeId> searchNodeIds;

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    for (const auto& searchVarId : implicitConstraint->outputVarNodeIds()) {
      searchNodeIds.emplace(searchVarId);
    }
  }

  for (const VarNode& vNode : _varNodes) {
    if (vNode.definingNodes().empty() &&
        (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty())) {
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
    std::unordered_set<VarNodeId> dynamicSearchNodeIds;
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

void InvariantGraph::createVars() {
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> visitedInvNodes;
  std::queue<InvariantNodeId> unregisteredInvNodes;

  // create a _solver var for each fixed boolean:
  for (const auto& varNodeId : _boolVarNodeIndices) {
    VarNode& vNode = varNode(varNodeId);
    if (vNode.staticInputTo().empty() && vNode.dynamicInputTo().empty()) {
      continue;
    }
    if (vNode.varId() == propagation::NULL_ID) {
      assert(vNode.constantValue().has_value());
      Int constant = *vNode.constantValue();
      vNode.setVarId(_solver.makeIntVar(constant, constant, constant));
    }
    for (int i = 0; i < 2; ++i) {
      for (const auto& listeningInvNode :
           i == 0 ? vNode.staticInputTo() : vNode.dynamicInputTo()) {
        if (!visitedInvNodes.contains(listeningInvNode)) {
          visitedInvNodes.emplace(listeningInvNode);
          unregisteredInvNodes.emplace(listeningInvNode);
        }
      }
    }
  }

  // create a _solver var for each fixed integer var
  for (const auto& [constant, varNodeId] : _intVarNodeIndices) {
    VarNode& vNode = varNode(varNodeId);
    if (vNode.staticInputTo().empty() && vNode.dynamicInputTo().empty()) {
      continue;
    }
    if (vNode.varId() == propagation::NULL_ID) {
      assert(vNode.constantValue().has_value() &&
             vNode.constantValue().value() == constant);
      vNode.setVarId(_solver.makeIntVar(constant, constant, constant));
    }
    for (int i = 0; i < 2; ++i) {
      for (const auto& listeningInvNode :
           i == 0 ? vNode.staticInputTo() : vNode.dynamicInputTo()) {
        if (!visitedInvNodes.contains(listeningInvNode)) {
          visitedInvNodes.emplace(listeningInvNode);
          unregisteredInvNodes.emplace(listeningInvNode);
        }
      }
    }
  }

  // create a _solver var for each other fixed variable:
  for (VarNode& vNode : _varNodes) {
    if (!vNode.definingNodes().empty() ||
        (vNode.staticInputTo().empty() && vNode.dynamicInputTo().empty())) {
      continue;
    }
    assert(vNode.isFixed());
    if (vNode.varId() == propagation::NULL_ID) {
      vNode.setVarId(_solver.makeIntVar(vNode.lowerBound(), vNode.upperBound(),
                                        vNode.lowerBound()));
    }
    for (int i = 0; i < 2; ++i) {
      for (const auto& listeningInvNode :
           i == 0 ? vNode.staticInputTo() : vNode.dynamicInputTo()) {
        if (!visitedInvNodes.contains(listeningInvNode)) {
          visitedInvNodes.emplace(listeningInvNode);
          unregisteredInvNodes.emplace(listeningInvNode);
        }
      }
    }
  }

  // enqueue each implicit constraint:
  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    if (implicitConstraint->state() == InvariantNodeState::ACTIVE) {
      visitedInvNodes.emplace(implicitConstraint->id());
      unregisteredInvNodes.emplace(implicitConstraint->id());
    }
  }

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

    // The output variables have not been created in the _solver yet:
    assert(std::all_of(invNode.outputVarNodeIds().begin(),
                       invNode.outputVarNodeIds().end(),
                       [&](VarNodeId outputVarNodeId) {
                         return varId(outputVarNodeId) == propagation::NULL_ID;
                       }));
    if (invNode.state() != InvariantNodeState::ACTIVE) {
      continue;
    }

    invNode.registerOutputVars();
    for (const auto& outputVarNodeId : invNode.outputVarNodeIds()) {
      assert(outputVarNodeId != NULL_NODE_ID);
      const auto& vn = varNode(outputVarNodeId);
      assert(std::any_of(
          vn.definingNodes().begin(), vn.definingNodes().end(),
          [&](InvariantNodeId defNodeId) { return defNodeId == invNodeId; }));

      assert(vn.varId() != propagation::NULL_ID);
      for (int i = 0; i < 2; ++i) {
        for (const auto& nextVarDefNode :
             i == 0 ? vn.staticInputTo() : vn.dynamicInputTo()) {
          if (!visitedInvNodes.contains(nextVarDefNode)) {
            visitedInvNodes.emplace(nextVarDefNode);
            unregisteredInvNodes.emplace(nextVarDefNode);
          }
        }
      }
    }
  }

  // Each invariant that is not reachable from a search variable is deactivated:
  for (auto& invNode : _invariantNodes) {
    if (invNode->state() == InvariantNodeState::ACTIVE &&
        !visitedInvNodes.contains(invNode->id())) {
      invNode->deactivate();
      assert(invNode->state() == InvariantNodeState::SUBSUMED);
    }
  }
}

void InvariantGraph::createImplicitConstraints() {
  for (auto& implicitConstraintNode : _implicitConstraintNodes) {
    if (implicitConstraintNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::all_of(implicitConstraintNode->outputVarNodeIds().begin(),
                         implicitConstraintNode->outputVarNodeIds().end(),
                         [&](VarNodeId varNodeId) {
                           return varId(varNodeId) != propagation::NULL_ID;
                         }));
      implicitConstraintNode->registerNode();
    }
  }
}

void InvariantGraph::createInvariants() {
  for (auto& invariantNode : _invariantNodes) {
    if (invariantNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::all_of(invariantNode->outputVarNodeIds().begin(),
                         invariantNode->outputVarNodeIds().end(),
                         [&](VarNodeId varNodeId) {
                           return varId(varNodeId) != propagation::NULL_ID;
                         }));
      invariantNode->registerNode();
    }
  }
}

propagation::VarViewId InvariantGraph::createViolations() {
  std::vector<propagation::VarViewId> violations;
  for (const auto& definingNode : _invariantNodes) {
    if (!definingNode->isReified() &&
        definingNode->violationVarId() != propagation::NULL_ID) {
      violations.emplace_back(definingNode->violationVarId());
    }
  }

  for (auto& vNode : _varNodes) {
    if (vNode.varId() != propagation::NULL_ID) {
      const propagation::VarViewId violationId =
          vNode.postDomainConstraint(_solver);
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
  const propagation::VarViewId totalViolation = _solver.makeIntVar(0, 0, 0);
  _solver.makeInvariant<propagation::Linear>(_solver, totalViolation,
                                             std::move(violations));
  return totalViolation;
}

void InvariantGraph::construct() {
  sanity(false);
  replaceInvariantNodes();
  sanity(false);
  replaceFixedVars();
  sanity(false);
  populateRootNode();
  sanity(false);
  splitMultiDefinedVars();
  sanity(true);
  breakSelfCycles();
  sanity(true);
  breakCycles();
  sanity(true);
  _solver.open();
  createVars();
  createImplicitConstraints();
  createInvariants();
  _solver.computeBounds();
  _totalViolationVarId = createViolations();
  if (_totalViolationVarId == propagation::NULL_ID ||
      _objectiveVarNodeId == NULL_NODE_ID) {
    auto& trueBoolVarNode = varNode(varNodeId(true));
    if (trueBoolVarNode.varId() == propagation::NULL_ID) {
      trueBoolVarNode.setVarId(_solver.makeIntVar(0, 0, 0));
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
    varNode(_objectiveVarNodeId).setVarId(_solver.makeIntVar(1, 1, 1));
  }
  assert(_objectiveVarNodeId != NULL_NODE_ID);
  assert(varId(_objectiveVarNodeId) != propagation::NULL_ID);
}

void InvariantGraph::close() { _solver.close(); }

void InvariantGraph::sanity(bool oneDefInv) {
#ifndef NDEBUG
  assert(varNodeConst(_boolVarNodeIndices.at(0)).isFixed());
  assert(varNodeConst(_boolVarNodeIndices.at(0)).inDomain(bool{false}));
  assert(varNodeConst(_boolVarNodeIndices.at(1)).isFixed());
  assert(varNodeConst(_boolVarNodeIndices.at(1)).inDomain(bool{true}));
  for (const auto& [constant, vId] : _intVarNodeIndices) {
    const VarNode& vNode = varNodeConst(vId);
    assert(vNode.isIntVar());
    assert(vNode.isFixed());
    assert(vNode.inDomain(constant));
  }
  for (const VarNode& vNode : _varNodes) {
    for (const InvariantNodeId& invNodeId : vNode.definingNodes()) {
      IInvariantNode& invNode = invariantNode(invNodeId);
      assert(std::any_of(
          invNode.outputVarNodeIds().begin(), invNode.outputVarNodeIds().end(),
          [&](const VarNodeId vId) { return vId == vNode.varNodeId(); }));
    }
    for (const InvariantNodeId& invNodeId : vNode.staticInputTo()) {
      IInvariantNode& invNode = invariantNode(invNodeId);
      assert(std::any_of(
          invNode.staticInputVarNodeIds().begin(),
          invNode.staticInputVarNodeIds().end(),
          [&](const VarNodeId vId) { return vId == vNode.varNodeId(); }));
    }
    for (const InvariantNodeId& invNodeId : vNode.dynamicInputTo()) {
      IInvariantNode& invNode = invariantNode(invNodeId);
      assert(std::any_of(
          invNode.dynamicInputVarNodeIds().begin(),
          invNode.dynamicInputVarNodeIds().end(),
          [&](const VarNodeId vId) { return vId == vNode.varNodeId(); }));
    }
    if (oneDefInv) {
      assert(vNode.definingNodes().size() <= 1);
    }
  }
  for (const auto& implNode : _implicitConstraintNodes) {
    for (const VarNodeId vId : implNode->outputVarNodeIds()) {
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
    for (const VarNodeId vId : invNode->outputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::any_of(vNode.definingNodes().begin(),
                         vNode.definingNodes().end(),
                         [&](const InvariantNodeId& invId) {
                           return invId == invNode->id();
                         }));
    }
    for (const VarNodeId vId : invNode->staticInputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::any_of(vNode.staticInputTo().begin(),
                         vNode.staticInputTo().end(),
                         [&](const InvariantNodeId& invId) {
                           return invId == invNode->id();
                         }));
    }
    for (const VarNodeId vId : invNode->dynamicInputVarNodeIds()) {
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

void InvariantGraph::writeDotFile(std::ostream& o) const {
  std::vector<bool> visitedVarNodes(_varNodes.size() + 1, false);

  o << "digraph G {" << std::endl;

  std::vector<std::string> varIdentifiers;
  varIdentifiers.reserve(_namedVarNodeIndices.size());
  for (const auto& [identifier, _] : _namedVarNodeIndices) {
    varIdentifiers.push_back(identifier);
  }

  std::sort(varIdentifiers.begin(), varIdentifiers.end());

  for (const std::string& identifier : varIdentifiers) {
    const auto& vNode = varNodeConst(identifier);
    visitedVarNodes.at(vNode.varNodeId()) = true;
    if (!vNode.staticInputTo().empty() || !vNode.dynamicInputTo().empty() ||
        !vNode.definingNodes().empty()) {
      vNode.dotLangIdentifier(o, identifier);
    }
  }

  size_t n = 1;

  for (size_t i = 1; i < _varNodes.size(); ++i) {
    if (visitedVarNodes.at(i)) {
      continue;
    }
    if (_varNodes.at(i).staticInputTo().empty() ||
        _varNodes.at(i).dynamicInputTo().empty() ||
        _varNodes.at(i).definingNodes().empty()) {
      continue;
    }
    const std::string identifier =
        _varNodes.at(i).isFixed()
            ? (_varNodes.at(i).isIntVar()
                   ? std::to_string(_varNodes.at(i).lowerBound())
                   : (_varNodes.at(i).inDomain(bool{false}) ? "false" : "true"))
            : ("ATLANTIS_" + std::to_string(n));
    _varNodes.at(i).dotLangIdentifier(o, identifier);
    if (!_varNodes.at(i).isFixed()) {
      ++n;
    }
  }

  for (const auto& impl : _implicitConstraintNodes) {
    if (impl->state() == InvariantNodeState::ACTIVE) {
      impl->dotLangEntry(o);
    }
  }

  for (const auto& inv : _invariantNodes) {
    if (inv->state() == InvariantNodeState::ACTIVE) {
      inv->dotLangEntry(o);
    }
  }

  for (const auto& impl : _implicitConstraintNodes) {
    if (impl->state() == InvariantNodeState::ACTIVE) {
      impl->dotLangEdges(o);
    }
  }

  for (const auto& inv : _invariantNodes) {
    if (inv->state() == InvariantNodeState::ACTIVE) {
      inv->dotLangEdges(o);
    }
  }

  o << '}' << std::endl;
}

}  // namespace atlantis::invariantgraph
