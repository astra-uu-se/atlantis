#include "atlantis/invariantgraph/solverMapping.hpp"

namespace atlantis::invariantgraph {

propagation::VarViewId SolverMapping::invariantIntermediateId(
    size_t id, size_t index) const {
  if (id >= _invariantIntermediateIds.size() ||
      index >= _invariantIntermediateIds[id].size()) {
    return propagation::NULL_ID;
  }
  return _invariantIntermediateIds[id][index];
}

propagation::VarViewId SolverMapping::setInvariantIntermediateId(
    size_t id, size_t index, propagation::VarViewId solverId) {
  assert(solverId != propagation::NULL_ID);
  if (id >= _invariantIntermediateIds.size()) {
    _invariantIntermediateIds.resize(id + 1,
                                     std::vector<propagation::VarViewId>{});
  }
  if (index >= _invariantIntermediateIds[id].size()) {
    _invariantIntermediateIds[id].resize(index + 1, propagation::NULL_ID);
  }
  return _invariantIntermediateIds[id][index] = solverId;
}

propagation::VarViewId SolverMapping::implicitIntermediateId(
    size_t id, size_t index) const {
  if (id >= _implicitIntermediateIds.size() ||
      index >= _implicitIntermediateIds[id].size()) {
    return propagation::NULL_ID;
  }
  return _implicitIntermediateIds[id][index];
}

propagation::VarViewId SolverMapping::setImplicitIntermediateId(
    size_t id, size_t index, propagation::VarViewId solverId) {
  assert(solverId != propagation::NULL_ID);
  if (id >= _implicitIntermediateIds.size()) {
    _implicitIntermediateIds.resize(id + 1,
                                    std::vector<propagation::VarViewId>{});
  }
  if (index >= _implicitIntermediateIds[id].size()) {
    _implicitIntermediateIds[id].resize(index + 1, propagation::NULL_ID);
  }
  return _implicitIntermediateIds[id][index] = solverId;
}

propagation::VarViewId SolverMapping::solverId(VarNodeId varNodeId) const {
  assert(varNodeId != NULL_NODE_ID);
  if (size_t(varNodeId) >= _solverIds.size()) {
    return propagation::NULL_ID;
  }
  return _solverIds[size_t(varNodeId)];
}
propagation::VarViewId SolverMapping::domainViolationId(
    VarNodeId varNodeId) const {
  assert(varNodeId != NULL_NODE_ID);
  if (size_t(varNodeId) >= _domainViolationIds.size()) {
    return propagation::NULL_ID;
  }
  return _domainViolationIds[size_t(varNodeId)];
}

propagation::VarViewId SolverMapping::totalViolationId() const {
  return _totalViolationId;
}

propagation::VarViewId SolverMapping::objectiveId() const {
  return _objectiveId;
}

void SolverMapping::setSolverId(const VarNodeId& varNodeId,
                                propagation::VarViewId solverId) {
  assert(varNodeId != NULL_NODE_ID);
  assert(solverId != propagation::NULL_ID);
  if (_solverIds.size() <= size_t(varNodeId)) {
    _solverIds.resize(size_t(varNodeId) + 1, propagation::NULL_ID);
  }
  _solverIds[varNodeId] = solverId;
}
void SolverMapping::setDomainViolationId(const VarNodeId& varNodeId,
                                         propagation::VarViewId solverId) {
  assert(varNodeId != NULL_NODE_ID);
  assert(solverId != propagation::NULL_ID);
  if (_domainViolationIds.size() <= size_t(varNodeId)) {
    _domainViolationIds.resize(size_t(varNodeId) + 1, propagation::NULL_ID);
  }
  _domainViolationIds[size_t(varNodeId)] = solverId;
}

void SolverMapping::setTotalViolationId(propagation::VarViewId solverId) {
  _totalViolationId = solverId;
}

void SolverMapping::setObjectiveId(propagation::VarViewId solverId) {
  _objectiveId = solverId;
}

bool SolverMapping::hasNeighborhood(const InvariantNodeId id) const {
  assert(id != NULL_NODE_ID);
  if (!id.isImplicitConstraint() || _neighborhoods.size() <= size_t(id)) {
    return false;
  }
  return _neighborhoods[size_t(id)] != nullptr;
}

std::shared_ptr<search::neighborhoods::Neighborhood>
SolverMapping::neighborhood(const InvariantNodeId id) {
  assert(id != NULL_NODE_ID);
  if (!id.isImplicitConstraint() || _neighborhoods.size() <= size_t(id)) {
    return {nullptr};
  }
  return _neighborhoods[size_t(id)];
}
propagation::VarViewId SolverMapping::violationId(InvariantNodeId id) const {
  assert(id != NULL_NODE_ID);
  if (id.isImplicitConstraint() || size_t(id) >= _violationIds.size()) {
    return propagation::NULL_ID;
  }
  return _violationIds[size_t(id)];
}
void SolverMapping::setViolationId(InvariantNodeId id,
                                   propagation::VarViewId solverId) {
  assert(id != NULL_NODE_ID);
  assert(solverId != propagation::NULL_ID);
  if (id.isImplicitConstraint()) {
    assert(false);
    return;
  }
  if (size_t(id) >= _violationIds.size()) {
    _violationIds.resize(size_t(id) + 1, propagation::NULL_ID);
  }
  _violationIds[size_t(id)] = solverId;
}
propagation::VarViewId SolverMapping::intermediateId(InvariantNodeId id,
                                                     size_t index) const {
  assert(id != NULL_NODE_ID);
  return id.isInvariant() ? invariantIntermediateId(size_t(id), index)
                          : implicitIntermediateId(size_t(id), index);
}
propagation::VarViewId SolverMapping::intermediateId(InvariantNodeId id) const {
  return intermediateId(id, 0);
}
propagation::VarViewId SolverMapping::setIntermediateId(
    InvariantNodeId id, size_t index, propagation::VarViewId solverId) {
  assert(id != NULL_NODE_ID);
  assert(solverId != propagation::NULL_ID);
  return id.isInvariant()
             ? setInvariantIntermediateId(size_t(id), index, solverId)
             : setImplicitIntermediateId(size_t(id), index, solverId);
}

propagation::VarViewId SolverMapping::setIntermediateId(
    InvariantNodeId id, propagation::VarViewId solverId) {
  assert(id != NULL_NODE_ID);
  assert(solverId != propagation::NULL_ID);
  return id.isInvariant() ? setInvariantIntermediateId(size_t(id), 0, solverId)
                          : setImplicitIntermediateId(size_t(id), 0, solverId);
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
    const InvariantNodeId id,
    const std::shared_ptr<search::neighborhoods::Neighborhood>& neighborhood) {
  assert(id != NULL_NODE_ID);
  assert(neighborhood != nullptr);
  if (!id.isImplicitConstraint()) {
    return false;
  }
  if (size_t(id) >= _neighborhoods.size()) {
    _neighborhoods.resize(size_t(id) + 1, nullptr);
  }
  _neighborhoods[size_t(id)] = neighborhood;
  return true;
}
Int SolverMapping::objectiveOptimalValue() const {
  return _objectiveOptimalValue;
}

void SolverMapping::setObjectiveOptimalValue(Int value) {
  _objectiveOptimalValue = value;
}
ObjectiveDirection SolverMapping::objectiveDirection() const {
  return _objectiveDirection;
}
void SolverMapping::setObjectiveDirection(ObjectiveDirection direction) {
  _objectiveDirection = direction;
}

}  // namespace atlantis::invariantgraph
