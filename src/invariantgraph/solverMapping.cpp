#include "atlantis/invariantgraph/solverMapping.hpp"

#include "atlantis/invariantgraph/implicitConstraintNode.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

propagation::VarViewId SolverMapping::invariantIntermediateId(
    const std::shared_ptr<const InvariantNode>& invNode, const size_t index) const {
  const auto& iter = _invariantIntermediateIds.find(invNode);
  if (iter == _invariantIntermediateIds.end()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  if (index >= iter->second.size()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  return iter->second[index];
}
propagation::VarViewId SolverMapping::invariantIntermediateId(
    InvariantNode& invNode, const size_t index) const {
  return invariantIntermediateId(invNode.ptrConst(), index);
}

propagation::VarViewId SolverMapping::setInvariantIntermediateId(
    const std::shared_ptr<const InvariantNode>& invNode, const size_t index,
    const propagation::VarViewId solverId) {
  assert(solverId != propagation::NULL_ID);
  const auto& iter = _invariantIntermediateIds.find(invNode);
  if (iter == _invariantIntermediateIds.end()) {
    const auto& [pair, success] = _invariantIntermediateIds.emplace(invNode,
                                     std::vector<propagation::VarViewId>(index + 1, propagation::VAR_VIEW_NULL_ID));
    assert(success);
    return pair->second[index] = solverId;
  }
  if (index >= iter->second.size()) {
    iter->second.resize(index + 1, propagation::VAR_VIEW_NULL_ID);
  }
  return iter->second[index] = solverId;
}

propagation::VarViewId SolverMapping::setInvariantIntermediateId(
    InvariantNode& invNode, size_t index, propagation::VarViewId solverId) {
  return setIntermediateId(invNode.ptrConst(), index, solverId);
}

propagation::VarViewId SolverMapping::implicitIntermediateId(
    const std::shared_ptr<const ImplicitConstraintNode>& implNode, const size_t index) const {
  const auto& iter = _implicitIntermediateIds.find(implNode);
  if (iter == _implicitIntermediateIds.end()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  if (index >= iter->second.size()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  return iter->second[index];
}

propagation::VarViewId SolverMapping::implicitIntermediateId(
    ImplicitConstraintNode& implNode, const size_t index) const {
  return implicitIntermediateId(std::dynamic_pointer_cast<const ImplicitConstraintNode>(implNode.ptrConst()), index);
}

propagation::VarViewId SolverMapping::setImplicitIntermediateId(
    const std::shared_ptr<const ImplicitConstraintNode>& implNode, const size_t index,
    const propagation::VarViewId solverId) {
  assert(solverId != propagation::NULL_ID);
  const auto& iter = _implicitIntermediateIds.find(implNode);
  if (iter == _implicitIntermediateIds.end()) {
    const auto& [pair, success] = _implicitIntermediateIds.emplace(implNode,
                                     std::vector<propagation::VarViewId>(index + 1, propagation::VAR_VIEW_NULL_ID));
    assert(success);
    return pair->second[index] = solverId;
  }
  if (index >= iter->second.size()) {
    iter->second.resize(index + 1, propagation::VAR_VIEW_NULL_ID);
  }
  return iter->second[index] = solverId;
}
propagation::VarViewId SolverMapping::setImplicitIntermediateId(
    ImplicitConstraintNode& implNode, const size_t index, const propagation::VarViewId solverId) {
  return setImplicitIntermediateId(std::dynamic_pointer_cast<const ImplicitConstraintNode>(implNode.ptrConst()), index, solverId);
}

propagation::VarViewId SolverMapping::solverId(const std::shared_ptr<const VarNode>& varNode) const {
  const auto& iter = _solverIds.find(varNode);
  if (iter == _solverIds.end()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  return iter->second;
}

propagation::VarViewId SolverMapping::solverId(const VarNode& varNode) const {
  return solverId(varNode.ptrConst());
}

propagation::VarViewId SolverMapping::domainViolationId(
    const std::shared_ptr<const VarNode>& varNode) const {
  const auto& iter = _domainViolationIds.find(varNode);
  if (iter == _solverIds.end()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  return iter->second;
}

propagation::VarViewId SolverMapping::domainViolationId(const VarNode& varNode) const {
  return domainViolationId(varNode.ptrConst());
}

propagation::VarViewId SolverMapping::totalViolationId() const {
  return _totalViolationId;
}

propagation::VarViewId SolverMapping::objectiveId() const {
  return _objectiveId;
}

void SolverMapping::setSolverId(const std::shared_ptr<const VarNode>& varNode,
                                const propagation::VarViewId solverId) {
  const auto iter = _solverIds.find(varNode);
  if (iter == _solverIds.end()) {
    _solverIds.emplace(varNode, solverId);
  } else {
    iter->second = solverId;
  }
  assert(_solverIds.contains(varNode));
  assert(_solverIds.find(varNode)->second == solverId);
}

void SolverMapping::setSolverId(const VarNode& varNode, propagation::VarViewId solverId) {
  return setSolverId(varNode.ptrConst(), solverId);
}

void SolverMapping::setDomainViolationId(
    const std::shared_ptr<const VarNode>& varNode, const propagation::VarViewId solverId) {
  const auto iter = _domainViolationIds.find(varNode);
  if (iter == _domainViolationIds.end()) {
    _domainViolationIds.emplace(varNode, solverId);
  } else {
    iter->second = solverId;
  }
  assert(_domainViolationIds.contains(varNode));
  assert(_domainViolationIds.find(varNode)->second == solverId);
}
void SolverMapping::setDomainViolationId(const VarNode& varNode,
                                         propagation::VarViewId solverId) {
  return setDomainViolationId(varNode.ptrConst(), solverId);
}

void SolverMapping::setTotalViolationId(const propagation::VarViewId solverId) {
  _totalViolationId = solverId;
}

void SolverMapping::setObjectiveId(const propagation::VarViewId solverId) {
  _objectiveId = solverId;
}

bool SolverMapping::hasNeighborhood(
    const std::shared_ptr<const ImplicitConstraintNode>&) const {}

bool SolverMapping::hasNeighborhood(const std::shared_ptr<const InvariantNode>& invNode) const {
  if (std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode) == nullptr) {
    return false;
  }
  const auto& iter = _neighborhoods.find(invNode);
  return iter != _neighborhoods.end() && iter->second != nullptr;
}

bool SolverMapping::hasNeighborhood(const ImplicitConstraintNode& implNode) const {
  return hasNeighborhood(std::dynamic_pointer_cast<const ImplicitConstraintNode>(implNode.ptrConst()));
}

bool SolverMapping::hasNeighborhood(InvariantNode& invNode) const {
  return hasNeighborhood(invNode.ptrConst());
}

std::shared_ptr<search::neighborhoods::Neighborhood>
SolverMapping::neighborhood(const std::shared_ptr<const InvariantNode>& invNode) {
  if (std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode) == nullptr) {
    return nullptr;
  }
  const auto& iter = _neighborhoods.find(invNode);
  if (iter == _neighborhoods.end()) {
    return nullptr;
  }
  return iter->second;
}

std::shared_ptr<search::neighborhoods::Neighborhood>
SolverMapping::neighborhood(InvariantNode& invNode) {
  return neighborhood(invNode.ptrConst());
}

propagation::VarViewId SolverMapping::violationId(const std::shared_ptr<const InvariantNode>& invNode) const {
  if (std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode) == nullptr) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  const auto& iter = _violationIds.find(invNode);
  if (iter == _violationIds.end()) {
    return propagation::VAR_VIEW_NULL_ID;
  }
  return iter->second;
}

propagation::VarViewId SolverMapping::violationId(
    const InvariantNode& invNode) const {
  return violationId(invNode.ptrConst());
}

void SolverMapping::setViolationId(const std::shared_ptr<const InvariantNode>& invNode,
                                   const propagation::VarViewId solverId) {
  assert(std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode) != nullptr);
  const auto& iter = _violationIds.find(invNode);
  if (iter == _violationIds.end()) {
    auto [pair, success] = _violationIds.emplace(invNode, solverId);
    assert(success);
    pair->second = solverId;
  } else {
    iter->second = solverId;
  }
  assert(_violationIds.contains(invNode));
  assert(_violationIds.find(invNode)->second == solverId);
}
void SolverMapping::setViolationId(const InvariantNode& invNode,
                                   const propagation::VarViewId solverId) {
  return setViolationId(invNode.ptrConst(), solverId);
}

propagation::VarViewId SolverMapping::intermediateId(const std::shared_ptr<const InvariantNode>& invNode,
                                                     const size_t index) const {
  const auto& implNode = std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode);
  if (implNode != nullptr) {
    return implicitIntermediateId(implNode, index);
  }
  return invariantIntermediateId(invNode, index);
}

propagation::VarViewId SolverMapping::intermediateId(const InvariantNode& invNode,
                                                     const size_t index) const {
  return intermediateId(invNode.ptrConst(), index);
}

propagation::VarViewId SolverMapping::intermediateId(const std::shared_ptr<const InvariantNode>& invNode) const {
  return intermediateId(invNode, 0);
}

propagation::VarViewId SolverMapping::intermediateId(
    const InvariantNode& invNode) const {
  return intermediateId(invNode.ptrConst());
}

propagation::VarViewId SolverMapping::setIntermediateId(
    const std::shared_ptr<const InvariantNode>& invNode, const size_t index, const propagation::VarViewId solverId) {
  assert(solverId != propagation::NULL_ID);
  const auto& implNode = std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode);
  if (implNode != nullptr) {
    return setImplicitIntermediateId(implNode, index, solverId);
  }
  return setInvariantIntermediateId(invNode, index, solverId);
}

propagation::VarViewId SolverMapping::setIntermediateId(
    const InvariantNode& invNode, const size_t index,
    const propagation::VarViewId solverId) {
  return setIntermediateId(invNode.ptrConst(), index, solverId);
}

propagation::VarViewId SolverMapping::setIntermediateId(
    const std::shared_ptr<const InvariantNode>& invNode, const propagation::VarViewId solverId) {
  assert(solverId != propagation::NULL_ID);
  const auto& implNode = std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode);
  if (implNode != nullptr) {
    return setImplicitIntermediateId(implNode, 0, solverId);
  }
  return setInvariantIntermediateId(invNode, 0, solverId);
}

propagation::VarViewId SolverMapping::setIntermediateId(
    const InvariantNode& invNode, propagation::VarViewId solverId) {
  return setIntermediateId(invNode.ptrConst(), solverId);
}

bool SolverMapping::hasGlobalNeighborhood() const {
  return _globalNeighborhood != nullptr;
}

std::shared_ptr<search::neighborhoods::Neighborhood>
SolverMapping::globalNeighborhood() {
  return _globalNeighborhood;
}

void SolverMapping::setGlobalNeighborhood(
    const std::shared_ptr<search::neighborhoods::Neighborhood>&
        globalNeighborhood) {
  assert(globalNeighborhood != nullptr);
  _globalNeighborhood = globalNeighborhood;
}

bool SolverMapping::setNeighborhood(
    const std::shared_ptr<const InvariantNode>& invNode,
    const std::shared_ptr<search::neighborhoods::Neighborhood>& neighborhood) {
  assert(neighborhood != nullptr);
  if (std::dynamic_pointer_cast<const ImplicitConstraintNode>(invNode) == nullptr) {
    return false;
  }
  auto iter = _neighborhoods.find(invNode);
  if (iter == _neighborhoods.end()) {
    const auto [pair, success] = _neighborhoods.emplace(invNode, neighborhood);
    assert(success);
    return success;
  }
  iter->second = neighborhood;
  return true;
}

bool SolverMapping::setNeighborhood(
    const InvariantNode& invNode,
    const std::shared_ptr<search::neighborhoods::Neighborhood>& neighborhood) {
  return setNeighborhood(invNode.ptrConst(), neighborhood);
}

Int SolverMapping::objectiveOptimalValue() const {
  return _objectiveOptimalValue;
}

void SolverMapping::setObjectiveOptimalValue(const Int value) {
  _objectiveOptimalValue = value;
}
ObjectiveDirection SolverMapping::objectiveDirection() const {
  return _objectiveDirection;
}
void SolverMapping::setObjectiveDirection(const ObjectiveDirection direction) {
  _objectiveDirection = direction;
}

}  // namespace atlantis::invariantgraph
