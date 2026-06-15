#include "atlantis/invariantgraph/invariantGraph.hpp"

#include <numeric>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/gecodeSolver.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/fznAst.hpp"

using atlantis::propagation::SolverBase;

namespace atlantis::invariantgraph {

static void SCCUtil(const InvariantGraph& graph, VarNodeId inputId,
                    std::vector<Int>& discoverTime, std::vector<Int>& lowTime,
                    std::vector<VarNodeId>& stack, std::vector<bool>& onStack,
                    Int& time,
                    std::vector<std::vector<VarNodeId>>& components) {
  assert(inputId < discoverTime.size());
  assert(discoverTime.size() == lowTime.size());
  assert(discoverTime.size() == onStack.size());
  assert(!onStack[inputId]);
  discoverTime[inputId] = lowTime[inputId] = time;
  ++time;
  stack.emplace_back(inputId);
  onStack[inputId] = true;

  assert(graph.varNodeConst(inputId).definingNodes().size() <= 1);
  for (size_t i = 0; i < 2; ++i) {
    for (const InvariantNodeId invId :
         i == 0 ? graph.varNodeConst(inputId).staticInputTo()
                : graph.varNodeConst(inputId).dynamicInputTo()) {
      for (const VarNodeId outputId :
           graph.invariantNodeConst(invId).outputVarNodeIds()) {
        if (discoverTime[outputId] < 0) {
          SCCUtil(graph, outputId, discoverTime, lowTime, stack, onStack, time,
                  components);
          lowTime[inputId] = std::min(lowTime[outputId], lowTime[inputId]);
        } else if (onStack[outputId]) {
          lowTime[inputId] = std::min(discoverTime[outputId], lowTime[inputId]);
        }
      }
    }
  }
  if (lowTime[inputId] == discoverTime[inputId]) {
    const bool inSCC = stack.back() != inputId;
    if (inSCC) {
      components.emplace_back();
      while (stack.back() != inputId) {
        onStack[stack.back()] = false;
        components.back().emplace_back(stack.back());
        stack.pop_back();
      }
    }
    onStack[inputId] = false;
    assert(stack.back() == inputId);
    if (inSCC) {
      components.back().emplace_back(inputId);
    }
    stack.pop_back();
  }
}

static std::vector<std::vector<VarNodeId>> SCC(const InvariantGraph& graph) {
  std::vector<Int> discoverTime(graph.nextVarNodeId(), -1);
  std::vector<Int> lowTime(graph.nextVarNodeId(), -1);
  std::vector<VarNodeId> stack;
  stack.reserve(graph.nextVarNodeId());
  std::vector<bool> onStack(graph.nextVarNodeId(), false);
  std::vector<std::vector<VarNodeId>> components;
  components.reserve(graph.nextVarNodeId());
  Int time = 0;
  for (const std::shared_ptr<ImplicitConstraintNode>& invNode :
       graph.implicitConstraintNodes()) {
    for (const VarNodeId searchVar : invNode->outputVarNodeIds()) {
      if (discoverTime[searchVar] < 0) {
        SCCUtil(graph, searchVar, discoverTime, lowTime, stack, onStack, time,
                components);
      }
    }
  }
  for (VarNodeId varId = 0; varId < graph.nextVarNodeId(); ++varId) {
    if (discoverTime[varId] < 0) {
      SCCUtil(graph, varId, discoverTime, lowTime, stack, onStack, time,
              components);
    }
  }
  assert(std::ranges::none_of(onStack, [&](const bool b) { return b; }));
  assert(
      std::ranges::all_of(discoverTime, [&](const Int t) { return t >= 0; }));
  return components;
}

static std::vector<VarNodeId> findCycle(
    const InvariantGraph& graph, const std::vector<VarNodeId>& component,
    const size_t componentIndex, const std::vector<size_t>& componentOfVar,
    bool findDynCycles) {
  std::vector<VarNodeId> stack;
  std::vector<Int> discoverTime(componentOfVar.size(), -1);
  std::vector<VarNodeId> outputOf(componentOfVar.size(), NULL_NODE_ID);
  stack.reserve(component.size());
  Int time = 0;
  for (const VarNodeId orig : component) {
    if (discoverTime[orig] >= 0) {
      continue;
    }
    discoverTime[orig] = time;
    ++time;
    stack.emplace_back(orig);

    while (!stack.empty()) {
      const VarNodeId outputId = stack.back();
      stack.pop_back();
      if (outputId >= componentOfVar.size()) {
        // This var has been added when breaking a cycle and cannot be in
        // another cycle.
        continue;
      }
      discoverTime[outputId] = discoverTime[orig];
      assert(graph.varNodeConst(outputId).definingNodes().size() <= 1);
      if (graph.varNodeConst(outputId).definingNodes().empty()) {
        continue;
      }
      const auto defInv = *graph.varNodeConst(outputId).definingNodes().begin();
      assert(defInv != NULL_NODE_ID);
      const auto& invNode = graph.invariantNodeConst(defInv);
      for (unsigned int i = 0; i < (findDynCycles ? 2 : 1); ++i) {
        for (const VarNodeId inputId :
             (i == 0 ? invNode.staticInputVarNodeIds()
                     : invNode.dynamicInputVarNodeIds())) {
          if (inputId >= componentOfVar.size() ||
              componentOfVar[inputId] != componentIndex) {
            // This var either: (i) was added when breaking a cycle or (ii) is
            // not in the current component.
            continue;
          }
          // what if outputId != NULL_NODE_ID
          outputOf[inputId] = outputId;
          if (discoverTime[inputId] == discoverTime[orig]) {
            std::vector<VarNodeId> cycle;
            cycle.reserve(component.size());
            cycle.emplace_back(inputId);
            for (VarNodeId vId = outputId;
                 vId != inputId && vId != NULL_NODE_ID; vId = outputOf[vId]) {
              assert(vId < componentOfVar.size());
              assert(discoverTime.at(vId) == discoverTime.at(orig));
              assert(componentOfVar.at(vId) == componentOfVar.at(orig));
              cycle.emplace_back(vId);
            }
            return cycle;
          }
          if (discoverTime[inputId] < 0) {
            stack.emplace_back(inputId);
          }
        }
      }
    }
  }
  return {};
}

static std::vector<VarNodeId> findStaticCycle(
    const InvariantGraph& graph, const std::vector<VarNodeId>& component,
    const size_t componentIndex, const std::vector<size_t>& componentOfVar) {
  return findCycle(graph, component, componentIndex, componentOfVar, false);
}

static std::vector<VarNodeId> findDynamicCycle(
    const InvariantGraph& graph, const std::vector<VarNodeId>& component,
    const size_t componentIndex, const std::vector<size_t>& componentOfVar) {
  return findCycle(graph, component, componentIndex, componentOfVar, true);
}

static std::pair<VarNodeId, InvariantNodeId> findPivotInCycle(
    const InvariantGraph& graph, const std::vector<VarNodeId>& cycle) {
  assert(cycle.size() > 1);
  assert(std::ranges::all_of(cycle, [&](const VarNodeId vId) {
    return graph.varNodeConst(vId).definingNodes().size() == 1;
  }));

  std::vector<size_t> candidateIndices(cycle.size());
  std::iota(candidateIndices.begin(), candidateIndices.end(), 0);
  std::ranges::sort(candidateIndices, [&](const size_t lhs, const size_t rhs) {
    return graph.varNodeConst(cycle[lhs]).constDomain()->size() <
           graph.varNodeConst(cycle[rhs]).constDomain()->size();
  });

  for (const size_t index : candidateIndices) {
    const VarNodeId pivot = cycle[index];
    const size_t domSize = graph.varNodeConst(pivot).constDomain()->size();
    if (domSize <= 1) {
      continue;
    }
    assert(!graph.varNodeConst(pivot).isFixed());

    for (const auto outputId : cycle) {
      const auto& outputNode = graph.varNodeConst(outputId);
      assert(outputNode.definingNodes().size() == 1);
      const InvariantNodeId invId = *outputNode.definingNodes().begin();
      const auto& invNode = graph.invariantNodeConst(invId);

      const bool definesOutput = std::ranges::any_of(
          invNode.outputVarNodeIds(),
          [&](const VarNodeId vId) { return vId == outputNode.varNodeId(); });
      const bool usesPivot =
          std::ranges::any_of(
              invNode.staticInputVarNodeIds(),
              [&](const VarNodeId vId) { return vId == pivot; }) ||
          std::ranges::any_of(
              invNode.dynamicInputVarNodeIds(),
              [&](const VarNodeId vId) { return vId == pivot; });
      if (definesOutput && usesPivot) {
        return {pivot, invId};
      }
    }
  }

  assert(false);
  return std::pair<VarNodeId, InvariantNodeId>{cycle.front(),
                                               InvariantNodeId{NULL_NODE_ID}};
}

InvariantGraphRoot& InvariantGraph::root() const {
  return dynamic_cast<InvariantGraphRoot&>(*_implicitConstraintNodes.front());
}

InvariantGraph::InvariantGraph(const bool breakDynamicCycles)
    : _varNodes{VarNode{VarNodeId{0}, false,
                        std::make_shared<SearchDomain>(std::vector<Int>{1})},
                VarNode{VarNodeId{1}, false,
                        std::make_shared<SearchDomain>(std::vector<Int>{0})}},
      _boolVarNodeIndices{VarNodeId{0}, VarNodeId{1}},
      _constraintSolver(std::make_shared<GecodeSolver>()),
      _breakDynamicCycles(breakDynamicCycles),
      _objectiveVarNodeId{NULL_NODE_ID} {
  for (const VarNodeId bVarId : _boolVarNodeIndices) {
    varNode(bVarId).setConstraintVarId(
        _constraintSolver->newBoolVar(varNode(bVarId).inDomain(true)));
  }
  addImplicitConstraintNode(std::make_shared<InvariantGraphRoot>(*this));
}

ConstraintSolver& InvariantGraph::constraintSolver() {
  return *_constraintSolver;
}

const ConstraintSolver& InvariantGraph::constraintSolverConst() const {
  return *_constraintSolver;
}

VarNodeId InvariantGraph::nextVarNodeId() const {
  return VarNodeId{_varNodes.size()};
}

bool InvariantGraph::containsVarNode(const std::string& identifier) const {
  return !_namedVarNodeIndices.empty() &&
         _namedVarNodeIndices.contains(identifier);
}

bool InvariantGraph::containsVarNode(const Int i) const {
  return !_intVarNodeIndices.empty() && _intVarNodeIndices.contains(i);
}

bool InvariantGraph::containsVarNode(bool) const { return true; }

VarNodeId InvariantGraph::retrieveBoolVarNode(const bool b) {
  assert(varNode(_boolVarNodeIndices.at(0)).inDomain(bool{false}));
  assert(varNode(_boolVarNodeIndices.at(1)).inDomain(bool{true}));
  return _boolVarNodeIndices.at(b ? 1 : 0);
}

VarNodeId InvariantGraph::retrieveBoolVarNode(const bool value,
                                              const bool forceNewVar) {
  if (!forceNewVar) {
    return retrieveBoolVarNode(value);
  }
  return _varNodes
      .emplace_back(
          nextVarNodeId(), false,
          std::make_shared<SearchDomain>(std::vector<Int>{value ? 0 : 1}),
          _constraintSolver->newBoolVar(value), DomainType::DOM_FIXED)
      .varNodeId();
}

VarNodeId InvariantGraph::retrieveBoolVarNode(const std::string& identifier,
                                              const DomainType domainType) {
  if (!containsVarNode(identifier)) {
    const VarNodeId nId =
        _varNodes
            .emplace_back(identifier, nextVarNodeId(), false,
                          _constraintSolver->newBoolVar(), domainType)
            .varNodeId();
    _namedVarNodeIndices.emplace(identifier, nId);
    return nId;
  }
  assert(!varNode(identifier).isIntVar());
  return _namedVarNodeIndices.at(identifier);
}

VarNodeId InvariantGraph::retrieveBoolVarNode(const DomainType domainType) {
  return _varNodes
      .emplace_back(nextVarNodeId(), false, _constraintSolver->newBoolVar(),
                    domainType)
      .varNodeId();
}

VarNodeId InvariantGraph::retrieveBoolVarNode(const bool b,
                                              const std::string& identifier) {
  const VarNodeId nId = retrieveBoolVarNode(b);
  if (!containsVarNode(identifier)) {
    _namedVarNodeIndices.emplace(identifier, nId);
  } else {
    const auto& var = varNode(identifier);
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

VarNodeId InvariantGraph::retrieveBoolVarNode(
    const std::shared_ptr<SearchDomain>& domain, const DomainType domainType) {
  if (domain->isFixed()) {
    return retrieveBoolVarNode(domain->lowerBound() == 0);
  }
  return _varNodes
      .emplace_back(nextVarNodeId(), false, domain,
                    _constraintSolver->newBoolVar(), domainType)
      .varNodeId();
}

VarNodeId InvariantGraph::retrieveIntVarNode(const Int value) {
  if (!containsVarNode(value)) {
    const VarNodeId nodeId =
        _varNodes
            .emplace_back(
                nextVarNodeId(), true,
                std::make_shared<SearchDomain>(std::vector<Int>{value}),
                _constraintSolver->newIntVar(value), DomainType::DOM_FIXED)
            .varNodeId();
    _intVarNodeIndices.emplace(value, nodeId);
    return nodeId;
  }
  const auto& var = varNode(value);
  if (!var.isIntVar()) {
    throw std::invalid_argument("Variable " + std::to_string(value) +
                                " is not an integer variable");
  }
  if (!var.isFixed() || !var.inDomain(value)) {
    throw std::invalid_argument("Variable " + std::to_string(value) +
                                " is not fixed to " + std::to_string(value));
  }
  return _intVarNodeIndices.at(value);
}

VarNodeId InvariantGraph::retrieveIntVarNode(const Int value,
                                             const bool forceNew) {
  if (!forceNew) {
    retrieveIntVarNode(value);
  }
  const VarNodeId nodeId =
      _varNodes
          .emplace_back(nextVarNodeId(), true,
                        std::make_shared<SearchDomain>(std::vector<Int>{value}),
                        _constraintSolver->newIntVar(value),
                        DomainType::DOM_FIXED)
          .varNodeId();
  if (!containsVarNode(value)) {
    _intVarNodeIndices.emplace(value, nodeId);
  }
  return nodeId;
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

VarNodeId InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& domain, const DomainType domainType) {
  if (domain->isFixed()) {
    return retrieveIntVarNode(domain->lowerBound());
  }
  return _varNodes
      .emplace_back(nextVarNodeId(), true, domain,
                    _constraintSolver->newIntVar(*domain), domainType)
      .varNodeId();
}

VarNodeId InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& domain) {
  return retrieveIntVarNode(domain, DomainType::DOM_DOMAIN);
}

VarNodeId InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& domain, const std::string& identifier,
    const DomainType domainType) {
  if (containsVarNode(identifier)) {
    const auto& node = varNode(identifier);
    if (!node.isIntVar()) {
      throw std::invalid_argument("Variable " + identifier +
                                  " is not an integer variable");
    }
    return node.varNodeId();
  }

  VarNodeId nId =
      domain->isFixed()
          ? retrieveIntVarNode(domain->lowerBound())
          : _varNodes
                .emplace_back(identifier, nextVarNodeId(), true, domain,
                              _constraintSolver->newIntVar(*domain), domainType)
                .varNodeId();

  assert(!containsVarNode(identifier));
  _namedVarNodeIndices.emplace(identifier, nId);
  return nId;
}

VarNodeId InvariantGraph::retrieveIntVarNode(
    const std::shared_ptr<SearchDomain>& dom, const std::string& identifier) {
  return retrieveIntVarNode(dom, identifier, DomainType::DOM_DOMAIN);
}

VarNode& InvariantGraph::varNode(const std::string& identifier) {
  assert(_namedVarNodeIndices.contains(identifier));
  assert(size_t{_namedVarNodeIndices.at(identifier)} < _varNodes.size());
  return _varNodes.at(size_t{_namedVarNodeIndices.at(identifier)});
}

VarNode& InvariantGraph::varNode(const VarNodeId id) {
  assert(size_t{id} < _varNodes.size());
  return _varNodes.at(size_t{id});
}

VarNode& InvariantGraph::varNode(const Int value) {
  assert(_intVarNodeIndices.contains(value));
  assert(size_t{_intVarNodeIndices.at(value)} < _varNodes.size());
  return _varNodes.at(size_t{_intVarNodeIndices.at(value)});
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
          const bool wasReplaced = invNode.replace();
          assert(wasReplaced);
          if (wasReplaced) {
            invNode.deactivate();
          }
        } else if (invNode.canBeMadeImplicit()) {
          const bool isImplicit = invNode.makeImplicit();
          assert(isImplicit);
          if (isImplicit) {
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

const InvariantNode& InvariantGraph::invariantNodeConst(
    InvariantNodeId id) const {
  if (id.isInvariant()) {
    assert(size_t(id) < _invariantNodes.size());
    return *_invariantNodes.at(size_t(id));
  }
  assert(size_t(id) < _implicitConstraintNodes.size());
  return static_cast<const InvariantNode&>(
      *_implicitConstraintNodes.at(size_t(id)));
}

const std::vector<std::shared_ptr<ImplicitConstraintNode>>&
InvariantGraph::implicitConstraintNodes() const {
  return _implicitConstraintNodes;
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

bool InvariantGraph::containsInvariantNode(InvariantNodeId id) const {
  return id.isInvariant() && size_t(id) < _invariantNodes.size();
}

bool InvariantGraph::containsImplicitConstraintNode(InvariantNodeId id) const {
  return id.isImplicitConstraint() &&
         size_t(id) < _implicitConstraintNodes.size();
}

InvariantNode& InvariantGraph::invariantNode(InvariantNodeId id) {
  if (id.isImplicitConstraint()) {
    assert(containsImplicitConstraintNode(id));
    return implicitConstraintNode(id);
  }
  assert(containsInvariantNode(id));
  return *_invariantNodes.at(size_t(id));
}

ImplicitConstraintNode& InvariantGraph::implicitConstraintNode(
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
    std::shared_ptr<InvariantNode>&& node) {
  if (node->state() == InvariantNodeState::SUBSUMED) {
    return InvariantNodeId(NULL_NODE_ID);
  }
  if (node->state() != InvariantNodeState::UNINITIALIZED) {
    throw InvariantGraphException(
        "InvariantGraph::addInvariantNode: invariant: \"" +
        node->dotLangIdentifier() + "\" already initialized.");
  }
  const InvariantNodeId id = nextInvariantNodeId();
  const auto& invNode = _invariantNodes.emplace_back(std::move(node));
  invNode->init(id);
  invNode->postConstraint();
  return invNode->id();
}

void InvariantGraph::replaceVarNode(VarNodeId oldNodeId, VarNodeId newNodeId) {
  if (oldNodeId == newNodeId) {
    return;
  }
  const VarNode& oldNode = varNode(oldNodeId);
  VarNode& newNode = varNode(newNodeId);
  newNode.domain()->removeAllValuesExcept(*oldNode.constDomain());
  while (!oldNode.definingNodes().empty()) {
    const InvariantNodeId invNodeId = *(oldNode.definingNodes().begin());
    invariantNode(invNodeId).replaceDefinedVar(oldNode.varNodeId(),
                                               newNode.varNodeId());
    assert(!oldNode.definingNodes().contains(invNodeId));
    assert(newNode.definingNodes().contains(invNodeId));
    assert(std::ranges::none_of(
        invariantNode(invNodeId).outputVarNodeIds().begin(),
        invariantNode(invNodeId).outputVarNodeIds().end(),
        [&](const VarNodeId id) { return id == oldNodeId; }));
    assert(std::ranges::any_of(
        invariantNode(invNodeId).outputVarNodeIds().begin(),
        invariantNode(invNodeId).outputVarNodeIds().end(),
        [&](const VarNodeId id) { return id == newNodeId; }));
  }
  assert(oldNode.definingNodes().empty());

  while (!oldNode.staticInputTo().empty()) {
    [[maybe_unused]] const InvariantNodeId invNodeId =
        oldNode.staticInputTo().front();
    invariantNode(oldNode.staticInputTo().front())
        .replaceStaticInputVarNode(oldNode.varNodeId(), newNode.varNodeId());
    assert(std::ranges::none_of(
        oldNode.staticInputTo().begin(), oldNode.staticInputTo().end(),
        [&](const InvariantNodeId id) { return id == invNodeId; }));
    assert(std::ranges::any_of(
        newNode.staticInputTo().begin(), newNode.staticInputTo().end(),
        [&](const InvariantNodeId id) { return id == invNodeId; }));
    assert(std::ranges::none_of(
        invariantNode(invNodeId).staticInputVarNodeIds().begin(),
        invariantNode(invNodeId).staticInputVarNodeIds().end(),
        [&](const VarNodeId id) { return id == oldNodeId; }));
    assert(std::ranges::any_of(
        invariantNode(invNodeId).staticInputVarNodeIds().begin(),
        invariantNode(invNodeId).staticInputVarNodeIds().end(),
        [&](const VarNodeId id) { return id == newNodeId; }));
  }
  assert(oldNode.staticInputTo().empty());

  while (!oldNode.dynamicInputTo().empty()) {
    [[maybe_unused]] const InvariantNodeId invNodeId =
        oldNode.dynamicInputTo().front();
    invariantNode(oldNode.dynamicInputTo().front())
        .replaceDynamicInputVarNode(oldNode.varNodeId(), newNode.varNodeId());
    assert(std::ranges::none_of(
        oldNode.dynamicInputTo().begin(), oldNode.dynamicInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(std::ranges::any_of(
        newNode.dynamicInputTo().begin(), newNode.dynamicInputTo().end(),
        [&](const InvariantNodeId& id) { return id == invNodeId; }));
    assert(std::ranges::none_of(
        invariantNode(invNodeId).dynamicInputVarNodeIds().begin(),
        invariantNode(invNodeId).dynamicInputVarNodeIds().end(),
        [&](const VarNodeId id) { return id == oldNodeId; }));
    assert(std::ranges::any_of(
        invariantNode(invNodeId).dynamicInputVarNodeIds().begin(),
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
  for (auto& id : std::views::values(_namedVarNodeIndices)) {
    if (id == oldNodeId) {
      id = newNodeId;
    }
  }
}

InvariantNodeId InvariantGraph::addImplicitConstraintNode(
    std::shared_ptr<ImplicitConstraintNode>&& node) {
  const InvariantNodeId id = nextImplicitNodeId();
  const auto& implNode = _implicitConstraintNodes.emplace_back(std::move(node));
  implNode->init(id);
  implNode->postConstraint();
  return implNode->id();
}

void InvariantGraph::createNeighborhood(SolverBase& solver,
                                        SolverMapping& mapping) const {
  if (mapping.hasGlobalNeighborhood()) {
    return;
  }
  std::vector<std::shared_ptr<search::neighborhoods::Neighborhood>>
      neighborhoods;
  neighborhoods.reserve(_implicitConstraintNodes.size());

  for (auto const& implicitConstraint : _implicitConstraintNodes) {
    if (!mapping.hasNeighborhood(implicitConstraint->id())) {
      implicitConstraint->registerNode(solver, mapping);
    }
    if (mapping.hasNeighborhood(implicitConstraint->id())) {
      neighborhoods.emplace_back(
          mapping.neighborhood(implicitConstraint->id()));
    }
  }
  if (neighborhoods.size() == 1) {
    mapping.setGlobalNeighborhood(neighborhoods.front());
  } else {
    mapping.setGlobalNeighborhood(
        std::make_shared<search::neighborhoods::NeighborhoodCombinator>(
            std::move(neighborhoods)));
  }
}

const VarNode& InvariantGraph::objectiveVarNode() const {
  return varNodeConst(_objectiveVarNodeId);
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
  // DO NOT emplace to _varNodes while doing this kind of iteration!

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
          std::make_shared<SearchDomain>(*(_varNodes[i].constDomain())),
          _varNodes[i].constraintVarId(),
          isFixed ? DomainType::DOM_FIXED : DomainType::DOM_NONE);

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

void InvariantGraph::breakSelfCycles() {
  const size_t end = _invariantNodes.size();
  for (size_t i = 0; i < end; ++i) {
    const auto& invNode = _invariantNodes[i];
    std::unordered_set<VarNodeId> visitedOutputs;
    visitedOutputs.reserve(invNode->outputVarNodeIds().size());
    for (const auto& outputVarId : invNode->outputVarNodeIds()) {
      if (visitedOutputs.contains(outputVarId)) {
        continue;
      }
      bool hasSelfCycle = false;
      for (size_t k = 0; k < (_breakDynamicCycles ? 2 : 1); ++k) {
        for (const auto& inputVarId : k == 0
                                          ? invNode->staticInputVarNodeIds()
                                          : invNode->dynamicInputVarNodeIds()) {
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
                .emplace_back(nextVarNodeId(),
                              varNodeConst(outputVarId).isIntVar(),
                              std::make_shared<SearchDomain>(
                                  varNodeConst(outputVarId).lowerBound(),
                                  varNodeConst(outputVarId).upperBound()),
                              varNodeConst(outputVarId).constraintVarId(),
                              DomainType::DOM_NONE)
                .varNodeId();
        invNode->replaceDefinedVar(outputVarId, newDefinedVar);
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

void InvariantGraph::breakCycles() {
  std::vector<std::vector<VarNodeId>> components = SCC(*this);
  if (components.empty()) {
    return;
  }

  std::vector<size_t> componentOfVar(_varNodes.size(), components.size());
  for (size_t c = 0; c < components.size(); ++c) {
    for (const VarNodeId vId : components[c]) {
      componentOfVar[size_t(vId)] = c;
    }
  }

  for (size_t c = 0; c < components.size(); ++c) {
    for (size_t i = 0; i < (_breakDynamicCycles ? 2 : 1); ++i) {
      while (true) {
        std::vector<VarNodeId> cycle =
            i == 0 ? findStaticCycle(*this, components[c], c, componentOfVar)
                   : findDynamicCycle(*this, components[c], c, componentOfVar);
        if (cycle.empty()) {
          break;
        }
        const auto [pivotId, invId] = findPivotInCycle(*this, cycle);
        assert(pivotId != NULL_NODE_ID);
        auto& invNode = invariantNode(invId);
        const VarNodeId newDefinedVar =
            _varNodes
                .emplace_back(nextVarNodeId(), varNodeConst(pivotId).isIntVar(),
                              std::make_shared<SearchDomain>(
                                  varNodeConst(pivotId).lowerBound(),
                                  varNodeConst(pivotId).upperBound()),
                              varNodeConst(pivotId).constraintVarId(),
                              DomainType::DOM_NONE)
                .varNodeId();
        invNode.replaceStaticInputVarNode(pivotId, newDefinedVar);
        invNode.replaceDynamicInputVarNode(pivotId, newDefinedVar);
        if (varNodeConst(pivotId).isIntVar()) {
          addInvariantNode(std::make_shared<IntAllEqualNode>(
              *this, pivotId, newDefinedVar, true, true));
        } else {
          addInvariantNode(std::make_shared<BoolAllEqualNode>(
              *this, pivotId, newDefinedVar, true, true));
        }
        assert(varNodeConst(newDefinedVar).definingNodes().empty());
        root().addSearchVarNode(newDefinedVar);
      }
    }
  }
}

void createVarsUtil(
    const InvariantGraph& graph, InvariantNodeId invNodeId,
    std::unordered_set<InvariantNodeId, InvariantNodeIdHash>& visitedInvNodes,
    std::unordered_set<InvariantNodeId, InvariantNodeIdHash>& onStack,
    SolverBase& solver, SolverMapping& mapping) {
  if (visitedInvNodes.contains(invNodeId)) {
    return;
  }
  visitedInvNodes.emplace(invNodeId);
  const InvariantNode& invNode = graph.invariantNodeConst(invNodeId);
  if (invNode.state() != InvariantNodeState::ACTIVE) {
    return;
  }
  onStack.emplace(invNodeId);
  for (const VarNodeId inputId : invNode.staticInputVarNodeIds()) {
    const auto& inputVar = graph.varNodeConst(inputId);
    for (const InvariantNodeId defInv : inputVar.definingNodes()) {
      assert(!onStack.contains(defInv));
      if (!visitedInvNodes.contains(defInv)) {
        createVarsUtil(graph, defInv, visitedInvNodes, onStack, solver,
                       mapping);
      }
    }
  }
  assert(std::ranges::none_of(
      invNode.staticInputVarNodeIds(),
      [&](const VarNodeId inputId) { return inputId == NULL_NODE_ID; }));
  invNode.registerOutputVars(solver, mapping);
  onStack.erase(invNodeId);
}

void InvariantGraph::createVars(SolverBase& solver,
                                SolverMapping& mapping) const {
  // create a _solver var for each fixed boolean:
  for (const auto& varNodeId : _boolVarNodeIndices) {
    const VarNode& vNode = varNodeConst(varNodeId);
    assert(vNode.definingNodes().empty());
    if (vNode.staticInputTo().empty() && vNode.dynamicInputTo().empty()) {
      continue;
    }
    if (mapping.solverId(vNode.varNodeId()) == propagation::NULL_ID) {
      assert(vNode.constantValue().has_value());
      const Int constant = *vNode.constantValue();
      mapping.setSolverId(vNode.varNodeId(),
                          solver.makeIntVar(constant, constant, constant));
    }
  }

  // create a _solver var for each fixed integer var
  for (const auto& [constant, varNodeId] : _intVarNodeIndices) {
    const VarNode& vNode = varNodeConst(varNodeId);
    assert(vNode.definingNodes().empty());
    if (vNode.staticInputTo().empty() && vNode.dynamicInputTo().empty()) {
      continue;
    }
    if (mapping.solverId(vNode.varNodeId()) == propagation::NULL_ID) {
      assert(vNode.constantValue().has_value() &&
             vNode.constantValue().value() == constant);
      mapping.setSolverId(vNode.varNodeId(),
                          solver.makeIntVar(constant, constant, constant));
    }
  }

  // create a _solver var for each other fixed variable:
  for (const VarNode& vNode : _varNodes) {
    if (!vNode.definingNodes().empty() ||
        (vNode.staticInputTo().empty() && vNode.dynamicInputTo().empty())) {
      continue;
    }
    // assert(vNode.isFixed());
    if (mapping.solverId(vNode.varNodeId()) == propagation::NULL_ID) {
      mapping.setSolverId(
          vNode.varNodeId(),
          solver.makeIntVar(vNode.lowerBound(), vNode.lowerBound(),
                            vNode.lowerBound()));
    }
  }

  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> visitedInvNodes;
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> onStack;
  visitedInvNodes.reserve(_invariantNodes.size() +
                          _implicitConstraintNodes.size());
  onStack.reserve(_invariantNodes.size() + _implicitConstraintNodes.size());

  for (const auto& implNode : _implicitConstraintNodes) {
    if (implNode->state() == InvariantNodeState::ACTIVE) {
      createVarsUtil(*this, implNode->id(), visitedInvNodes, onStack, solver,
                     mapping);
    }
  }

  for (const auto& invNode : _invariantNodes) {
    if (invNode->state() == InvariantNodeState::ACTIVE) {
      createVarsUtil(*this, invNode->id(), visitedInvNodes, onStack, solver,
                     mapping);
    }
  }
}

void InvariantGraph::createImplicitConstraints(SolverBase& solver,
                                               SolverMapping& mapping) const {
  for (const auto& implicitConstraintNode : _implicitConstraintNodes) {
    if (implicitConstraintNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::ranges::all_of(
          implicitConstraintNode->outputVarNodeIds().begin(),
          implicitConstraintNode->outputVarNodeIds().end(),
          [&](VarNodeId varNodeId) {
            return mapping.solverId(varNodeId) != propagation::NULL_ID;
          }));
      implicitConstraintNode->registerNode(solver, mapping);
    }
  }
}

void InvariantGraph::createInvariants(SolverBase& solver,
                                      SolverMapping& mapping) const {
  for (const auto& invariantNode : _invariantNodes) {
    if (invariantNode->state() == InvariantNodeState::ACTIVE) {
      assert(std::ranges::all_of(
          invariantNode->outputVarNodeIds().begin(),
          invariantNode->outputVarNodeIds().end(), [&](VarNodeId varNodeId) {
            return mapping.solverId(varNodeId) != propagation::NULL_ID;
          }));
      invariantNode->registerNode(solver, mapping);
    }
  }
}

propagation::VarViewId InvariantGraph::createViolations(
    SolverBase& solver, SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> violations;
  for (const auto& definingNode : _invariantNodes) {
    if (definingNode->state() == InvariantNodeState::ACTIVE &&
        !definingNode->isReified() &&
        definingNode->violationVarId(mapping) != propagation::NULL_ID) {
      violations.emplace_back(definingNode->violationVarId(mapping));
    }
  }

  for (auto& vNode : _varNodes) {
    if (mapping.solverId(vNode.varNodeId()) != propagation::NULL_ID) {
      const propagation::VarViewId violationId =
          vNode.postDomainConstraint(solver, mapping);
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
  const propagation::VarViewId totalViolation = solver.makeIntVar(0, 0, 0);
  solver.makeInvariant<propagation::Linear>(solver, totalViolation,
                                            std::move(violations));
  return totalViolation;
}

SolverMapping InvariantGraph::construct(SolverBase& solver) const {
  const bool wasClosed = !solver.isOpen();
  if (wasClosed) {
    solver.open();
  }
  SolverMapping mapping;
  createVars(solver, mapping);
  createImplicitConstraints(solver, mapping);
  createInvariants(solver, mapping);
  createNeighborhood(solver, mapping);
  solver.computeBounds();
  mapping.setTotalViolationId(createViolations(solver, mapping));
  mapping.setObjectiveDirection(_objectiveDirection);
  if (_objectiveDirection != ObjectiveDirection::NONE &&
      _objectiveVarNodeId != NULL_NODE_ID) {
    mapping.setObjectiveId(mapping.solverId(_objectiveVarNodeId));
    mapping.setObjectiveOptimalValue(_objectiveDirection ==
                                             ObjectiveDirection::MINIMIZE
                                         ? objectiveVarNode().lowerBound()
                                         : objectiveVarNode().upperBound());
  } else {
    mapping.setObjectiveOptimalValue(0);
  }

  if (wasClosed) {
    solver.close();
  }
  return mapping;
}

void InvariantGraph::open() { _isOpen = true; }

void InvariantGraph::close() {
  if (!isOpen()) {
    return;
  }
  _constraintSolver->fixPoint();
  sanity(false);
  updateDomains();
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
}

void InvariantGraph::updateDomains() {
  std::array<std::vector<std::shared_ptr<SearchDomain>>, 2> domains{
      std::vector<std::shared_ptr<SearchDomain>>(
          _constraintSolver->numIntVars(), nullptr),
      std::vector<std::shared_ptr<SearchDomain>>(
          _constraintSolver->numBoolVars(), nullptr)};
  for (size_t i = 0; i < _constraintSolver->numIntVars(); i++) {
    domains[0][i] = std::make_shared<SearchDomain>(
        _constraintSolver->intVarDomain({i, true}));
  }
  for (size_t i = 0; i < _constraintSolver->numBoolVars(); i++) {
    domains[1][i] = std::make_shared<SearchDomain>(
        _constraintSolver->boolVarDomain({i, false}));
  }

  for (auto& varNode : _varNodes) {
    if (varNode.isIntVar()) {
      varNode.replaceDomain(domains[0][size_t{varNode.constraintVarId()}]);
    } else {
      varNode.replaceDomain(domains[1][size_t{varNode.constraintVarId()}]);
    }
  }
}

void InvariantGraph::sanity([[maybe_unused]] bool oneDefInv) {
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
      InvariantNode& invNode = invariantNode(invNodeId);
      assert(std::ranges::any_of(
          invNode.outputVarNodeIds(),
          [&](const VarNodeId vId) { return vId == vNode.varNodeId(); }));
    }
    for (const InvariantNodeId& invNodeId : vNode.staticInputTo()) {
      InvariantNode& invNode = invariantNode(invNodeId);
      assert(std::ranges::any_of(
          invNode.staticInputVarNodeIds(),
          [&](const VarNodeId vId) { return vId == vNode.varNodeId(); }));
    }
    for (const InvariantNodeId& invNodeId : vNode.dynamicInputTo()) {
      InvariantNode& invNode = invariantNode(invNodeId);
      assert(std::ranges::any_of(
          invNode.dynamicInputVarNodeIds(),
          [&](const VarNodeId vId) { return vId == vNode.varNodeId(); }));
    }
    if (oneDefInv) {
      assert(vNode.definingNodes().size() <= 1);
    }
  }
  for (const auto& implNode : _implicitConstraintNodes) {
    for (const VarNodeId vId : implNode->outputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::ranges::any_of(vNode.definingNodes(),
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
      assert(std::ranges::any_of(vNode.definingNodes(),
                                 [&](const InvariantNodeId& invId) {
                                   return invId == invNode->id();
                                 }));
    }
    for (const VarNodeId vId : invNode->staticInputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::ranges::any_of(vNode.staticInputTo(),
                                 [&](const InvariantNodeId& invId) {
                                   return invId == invNode->id();
                                 }));
    }
    for (const VarNodeId vId : invNode->dynamicInputVarNodeIds()) {
      const VarNode& vNode = varNode(vId);
      assert(std::ranges::any_of(vNode.dynamicInputTo(),
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
  for (const auto& identifier : std::views::keys(_namedVarNodeIndices)) {
    varIdentifiers.push_back(identifier);
  }

  std::ranges::sort(varIdentifiers.begin(), varIdentifiers.end());

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
