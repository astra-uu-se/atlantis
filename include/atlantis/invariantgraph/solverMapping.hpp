#pragma once
#include <memory>
#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::invariantgraph {

class SolverMapping {
  std::vector<propagation::VarViewId> _solverIds{};
  std::vector<propagation::VarViewId> _domainViolationIds{};
  std::vector<std::vector<propagation::VarViewId>> _invariantIntermediateIds{};
  std::vector<std::vector<propagation::VarViewId>> _implicitIntermediateIds{};
  std::vector<std::shared_ptr<search::neighborhoods::Neighborhood>>
      _neighborhoods{};
  std::shared_ptr<search::neighborhoods::Neighborhood> _globalNeighborhood{
      nullptr};
  std::vector<propagation::VarViewId> _violationIds{};
  propagation::VarViewId _totalViolationId{propagation::NULL_ID};
  propagation::VarViewId _objectiveId{propagation::NULL_ID};
  Int _objectiveOptimalValue{0};
  ObjectiveDirection _objectiveDirection{ObjectiveDirection::NONE};

  [[nodiscard]] propagation::VarViewId invariantIntermediateId(
      size_t id, size_t index) const;

  propagation::VarViewId setInvariantIntermediateId(
      size_t id, size_t index, propagation::VarViewId solverId);

  [[nodiscard]] propagation::VarViewId implicitIntermediateId(
      size_t id, size_t index) const;

  propagation::VarViewId setImplicitIntermediateId(
      size_t id, size_t index, propagation::VarViewId solverId);

 public:
  SolverMapping() = default;

  [[nodiscard]] propagation::VarViewId solverId(VarNodeId varNodeId) const;

  [[nodiscard]] propagation::VarViewId domainViolationId(
      VarNodeId varNodeId) const;

  [[nodiscard]] propagation::VarViewId totalViolationId() const;

  [[nodiscard]] propagation::VarViewId objectiveId() const;

  void setSolverId(const VarNodeId& varNodeId, propagation::VarViewId solverId);

  void setDomainViolationId(const VarNodeId& varNodeId,
                            propagation::VarViewId solverId);

  void setTotalViolationId(propagation::VarViewId solverId);

  void setObjectiveId(propagation::VarViewId solverId);

  [[nodiscard]] bool hasNeighborhood(InvariantNodeId id) const;

  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  neighborhood(InvariantNodeId id);

  [[nodiscard]] propagation::VarViewId violationId(InvariantNodeId id) const;

  void setViolationId(InvariantNodeId id, propagation::VarViewId solverId);

  [[nodiscard]] propagation::VarViewId intermediateId(InvariantNodeId id,
                                                      size_t index) const;

  [[nodiscard]] propagation::VarViewId intermediateId(InvariantNodeId id) const;

  propagation::VarViewId setIntermediateId(InvariantNodeId id, size_t index,
                                           propagation::VarViewId solverId);

  propagation::VarViewId setIntermediateId(InvariantNodeId id,
                                           propagation::VarViewId solverId);

  [[nodiscard]] bool hasGlobalNeighborhood() const;

  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  globalNeighborhood();

  void setGlobalNeighborhood(
      const std::shared_ptr<search::neighborhoods::Neighborhood>&);

  bool setNeighborhood(
      InvariantNodeId id,
      const std::shared_ptr<search::neighborhoods::Neighborhood>&);

  [[nodiscard]] Int objectiveOptimalValue() const;

  void setObjectiveOptimalValue(Int value);

  [[nodiscard]] ObjectiveDirection objectiveDirection() const;

  void setObjectiveDirection(ObjectiveDirection);
};

}  // namespace atlantis::invariantgraph