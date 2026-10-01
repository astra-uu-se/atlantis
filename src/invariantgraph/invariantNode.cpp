#include "atlantis/invariantgraph/invariantNode.hpp"

#include <cassert>
#include <fznparser/model.hpp>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a soft constraint (which defines a violation), or a view.
 */
InvariantNode::InvariantNode(InvariantGraph& invariantGraph,
                             std::vector<std::shared_ptr<VarNode>>&& outputIds,
                             std::vector<std::shared_ptr<VarNode>>&& staticInputIds,
                             std::vector<std::shared_ptr<VarNode>>&& dynamicInputIds)
    : _invariantGraph(invariantGraph),
      _outputVarNodes(std::move(outputIds)),
      _staticInputVarNodes(std::move(staticInputIds)),
      _dynamicInputVarNodes(std::move(dynamicInputIds)) {}

std::shared_ptr<InvariantNode> InvariantNode::getPtr() {
  return shared_from_this();
}

InvariantGraph& InvariantNode::invariantGraph() { return _invariantGraph; }

ConstraintSolver& InvariantNode::constraintSolver() const {
  return _invariantGraph.constraintSolver();
}

void InvariantNode::setState(const InvariantNodeState state) { _state = state; }

const InvariantGraph& InvariantNode::invariantGraphConst() const {
  return _invariantGraph;
}

const ConstraintSolver& InvariantNode::constraintSolverConst() const {
  return _invariantGraph.constraintSolverConst();
}

void InvariantNode::postConstraint() {}

bool InvariantNode::isReified() const { return false; }

bool InvariantNode::isViolationInvariant() const { return false; }

void InvariantNode::updateState() {}

bool InvariantNode::canBeReplaced() const { return false; }

bool InvariantNode::replace() { return false; }

std::pair<size_t, size_t> InvariantNode::implicitRank() const { return {0, 0}; }

bool InvariantNode::canBeMadeImplicit() const { return false; }

bool InvariantNode::makeImplicit() { return false; }

InvariantNodeState InvariantNode::state() const { return _state; }

const std::vector<std::shared_ptr<VarNode>>& InvariantNode::outputVarNodes() const {
  return _outputVarNodes;
}
const std::vector<std::shared_ptr<VarNode>>& InvariantNode::staticInputVarNodes() const {
  return _staticInputVarNodes;
}
const std::vector<std::shared_ptr<VarNode>>& InvariantNode::dynamicInputVarNodes() const {
  return _dynamicInputVarNodes;
}

void InvariantNode::init() {
  if (_state != InvariantNodeState::UNINITIALIZED) {
    return;
  }
  for (const auto& varNode : _outputVarNodes) {
    markOutputTo(varNode->ptr(), false);
  }
  for (const auto& varNode : _staticInputVarNodes) {
    markStaticInputTo(varNode->ptr(), false);
  }
  for (const auto& varNode : _dynamicInputVarNodes) {
    markDynamicInputTo(varNode->ptr(), false);
  }
  _state = InvariantNodeState::ACTIVE;
}
void InvariantNode::setMappingId(const InvariantNodeId id) {
  _mappingId = id;
}

InvariantNodeId InvariantNode::mappingId() const { return _mappingId; }

propagation::VarViewId InvariantNode::violationVarId(
    const SolverMapping&) const {
  return propagation::VAR_VIEW_NULL_ID;
}

void InvariantNode::eraseStaticInputVarNode(const size_t index) {
  if (index >= _staticInputVarNodes.size()) {
    throw InvariantGraphException(
        "InvariantNode::eraseStaticInputVarNode: index out of bounds");
  }
  _staticInputVarNodes.erase(_staticInputVarNodes.begin() +
                               static_cast<Int>(index));
}

void InvariantNode::eraseDynamicInputVarNode(const size_t index) {
  if (index >= _dynamicInputVarNodes.size()) {
    throw InvariantGraphException(
        "InvariantNode::eraseDynamicInputVarNode: index out of bounds");
  }
  _dynamicInputVarNodes.erase(_dynamicInputVarNodes.begin() +
                                static_cast<Int>(index));
}

void InvariantNode::deactivate() {
  while (!staticInputVarNodes().empty()) {
    removeStaticInputVarNode(*staticInputVarNodes().front());
  }
  while (!dynamicInputVarNodes().empty()) {
    removeDynamicInputVarNode(*dynamicInputVarNodes().front());
  }
  while (!outputVarNodes().empty()) {
    removeOutputVarNode(*outputVarNodes().front());
  }
  setState(InvariantNodeState::SUBSUMED);
}

void InvariantNode::replaceDefinedVar(VarNode& oldOutputVarNode,
                                      const std::shared_ptr<VarNode>& newOutputVarNode) {
  // Replace all occurrences:
  for (auto& _outputVarNodeId : _outputVarNodes) {
    if (_outputVarNodeId.get() == &oldOutputVarNode) {
      _outputVarNodeId = newOutputVarNode;
    }
  }
  oldOutputVarNode.unmarkOutputTo(getPtr());
  newOutputVarNode->markOutputTo(getPtr());
}

void InvariantNode::removeStaticInputVarNode(
    VarNode&  staticInput) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_staticInputVarNodes.size()) - 1; i >= 0;
       --i) {
    if (_staticInputVarNodes[i].get() == &staticInput) {
      _staticInputVarNodes.erase(_staticInputVarNodes.begin() + i);
    }
  }
  staticInput.unmarkAsInputFor(*this, true);
}

void InvariantNode::removeStaticInputAtIndex(const size_t index) {
  // remove all occurrences:
  assert(index < _staticInputVarNodes.size());
  const std::shared_ptr<VarNode> varNode = _staticInputVarNodes[index];
  _staticInputVarNodes.erase(_staticInputVarNodes.begin() +
                               static_cast<Int>(index));
  const bool shouldUnmark = std::ranges::none_of(
      _staticInputVarNodes, [&](const std::shared_ptr<VarNode>& other) { return other == varNode; });
  if (shouldUnmark) {
    varNode->unmarkAsInputFor(*this, true);
  }
}

void InvariantNode::removeDynamicInputVarNode(
    VarNode& dynamicInput) {
  // remove all occurrences:
  for (Int i = static_cast<Int>(_dynamicInputVarNodes.size()) - 1; i >= 0;
       --i) {
    if (_dynamicInputVarNodes[i].get() == &dynamicInput) {
      _dynamicInputVarNodes.erase(_dynamicInputVarNodes.begin() + i);
    }
  }
  dynamicInput.unmarkAsInputFor(*this, false);
}

void InvariantNode::removeDynamicInputAtIndex(const size_t index) {
  // remove all occurrences:
  assert(index < _dynamicInputVarNodes.size());
  const std::shared_ptr<VarNode> varNode = _dynamicInputVarNodes[index];
  _dynamicInputVarNodes.erase(_dynamicInputVarNodes.begin() +
                               static_cast<Int>(index));
  const bool shouldUnmark = std::ranges::none_of(
      _staticInputVarNodes, [&](const std::shared_ptr<VarNode>& other) { return other == varNode; });
  if (shouldUnmark) {
    varNode->unmarkAsInputFor(*this, false);
  }
}

void InvariantNode::removeOutputVarNode(VarNode& output) {
  // remove all occurrences:
  bool didErase = false;
  for (Int i = static_cast<Int>(_outputVarNodes.size()) - 1; i >= 0; --i) {
    if (_outputVarNodes[i].get() == &output) {
      _outputVarNodes.erase(_outputVarNodes.begin() + i);
      didErase = true;
    }
  }
  if (!didErase) {
    return;
  }
  output.unmarkOutputTo(getPtr());
  if (_outputVarNodes.empty() && !isViolationInvariant()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void InvariantNode::removeOutputAtIndex(const size_t index) {
  // remove all occurrences:
  assert(index < _outputVarNodes.size());
  const std::shared_ptr<VarNode> varNode = _outputVarNodes[index];
  _outputVarNodes.erase(_outputVarNodes.begin() + static_cast<Int>(index));
  const bool shouldUnmark = std::ranges::none_of(
      _outputVarNodes, [&](const std::shared_ptr<VarNode>& other) { return other.get() == varNode.get(); });
  if (shouldUnmark) {
    varNode->unmarkOutputTo(getPtr());
  }
  if (_outputVarNodes.empty() && !isViolationInvariant()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void InvariantNode::replaceStaticInputVarNode(
    VarNode& oldStaticVar, const std::shared_ptr<VarNode>& newStaticVar) {
  // Replace all occurrences:
  bool wasInput = false;
  for (auto& sVarId : _staticInputVarNodes) {
    if (sVarId.get() == &oldStaticVar) {
      sVarId = newStaticVar;
      wasInput = true;
    }
  }
  assert(wasInput ==
         std::ranges::any_of(
             oldStaticVar.staticInputTo(),
             [&](const std::shared_ptr<InvariantNode>& other) { return other.get() == this; }));
  if (wasInput) {
    oldStaticVar.unmarkAsInputFor(*this, true);
    newStaticVar->markAsInputFor(getPtr(), true);
  }
}

void InvariantNode::replaceDynamicInputVarNode(
    VarNode& oldDynamicVar, const std::shared_ptr<VarNode>& newDynamicVar) {
  // Replace all occurrences:
  bool wasInput = false;
  for (auto& dVarId : _dynamicInputVarNodes) {
    if (dVarId.get() == &oldDynamicVar) {
      dVarId = newDynamicVar;
      wasInput = true;
    }
  }
  assert(wasInput ==
         std::ranges::any_of(
             oldDynamicVar.dynamicInputTo(),
             [&](const std::shared_ptr<InvariantNode>& other) { return other.get() == this; }));
  if (wasInput) {
    oldDynamicVar.unmarkAsInputFor(*this, false);
    newDynamicVar->markAsInputFor(getPtr(), false);
  }
}

std::ostream& InvariantNode::dotLangEntry(std::ostream& o) const {
  return o << reinterpret_cast<size_t>(this) << "[shape=box,label=\"" << dotLangIdentifier() << "\"];"
           << std::endl;
}

std::ostream& InvariantNode::dotLangEdges(std::ostream& o) const {
  for (const auto& varNode : staticInputVarNodes()) {
    o << reinterpret_cast<size_t>(varNode.get()) << " -> " << reinterpret_cast<size_t>(this) << "[style=solid];" << std::endl;
  }
  for (const auto& varNode : dynamicInputVarNodes()) {
    o << reinterpret_cast<size_t>(varNode.get()) << " -> " << reinterpret_cast<size_t>(this) << "[style=dashed];" << std::endl;
  }
  for (const auto& varNode : outputVarNodes()) {
    o << reinterpret_cast<size_t>(this) << " -> " << reinterpret_cast<size_t>(varNode.get()) << "[style = solid];" << std::endl;
  }
  return o;
}

std::vector<std::pair<std::shared_ptr<VarNode>, std::shared_ptr<VarNode>>>
InvariantNode::splitOutputVarNodes() {
  std::vector<std::pair<std::shared_ptr<VarNode>, std::shared_ptr<VarNode>>> replaced;
  replaced.reserve(_outputVarNodes.size());

  for (size_t i = 0; i < _outputVarNodes.size(); ++i) {
    for (size_t j = i + 1; j < _outputVarNodes.size(); ++j) {
      if (_outputVarNodes[i] != _outputVarNodes[j]) {
        continue;
      }
      if (_outputVarNodes[i]->isFixed()) {
        if (_outputVarNodes[i]->isIntVar()) {
          _outputVarNodes[j] =
              _invariantGraph.retrieveIntVarNode(_outputVarNodes[i]->lowerBound(), true);
        } else {
          _outputVarNodes[j] = _invariantGraph.retrieveBoolVarNode(
              _outputVarNodes[i]->inDomain(bool{true}), true);
        }
      } else if (_outputVarNodes[i]->isIntVar()) {
        _outputVarNodes[j] = _invariantGraph.retrieveIntVarNode(
            _outputVarNodes[i]->domain(), _outputVarNodes[i]->domainType());
      } else {
        _outputVarNodes[j] = _invariantGraph.retrieveBoolVarNode();
      }
      _outputVarNodes[j]->markOutputTo(*this);
      replaced.emplace_back(_outputVarNodes[i], _outputVarNodes[j]);
    }
  }
  return replaced;
}

propagation::VarViewId InvariantNode::makeSolverVar(
    const VarNode& varNode, const Int initialValue,
    propagation::SolverBase& solver, SolverMapping& mapping) const {
  if (mapping.solverId(varNode.mappingId()) == propagation::NULL_ID) {
    mapping.setSolverId(
        varNode.mappingId(), solver.makeIntVar(
                       std::max(varNode.lowerBound(),
                                std::min(varNode.upperBound(), initialValue)),
                       varNode.lowerBound(), varNode.upperBound()));
  }
  return mapping.solverId(varNode.mappingId());
}

propagation::VarViewId InvariantNode::makeSolverVar(
    const VarNode& varNode, propagation::SolverBase& solver,
    SolverMapping& mapping) const {
  return makeSolverVar(varNode, 0, solver, mapping);
}

void InvariantNode::markOutputTo(const std::shared_ptr<VarNode>& varNode,
                                 const bool registerHere) {
  varNode->markOutputTo(*this);

  if (registerHere) {
    _outputVarNodes.push_back(varNode);
  }
}

void InvariantNode::markStaticInputTo(const std::shared_ptr<VarNode>& varNode,
                                 const bool registerHere) {
  varNode->markAsInputFor(*this, true);

  if (registerHere) {
    _staticInputVarNodes.push_back(varNode);
  }
}

void InvariantNode::markDynamicInputTo(const std::shared_ptr<VarNode>& varNode,
                                 const bool registerHere) {
  varNode->markAsInputFor(*this, false);

  if (registerHere) {
    _dynamicInputVarNodes.push_back(varNode);
  }
}
}  // namespace atlantis::invariantgraph
