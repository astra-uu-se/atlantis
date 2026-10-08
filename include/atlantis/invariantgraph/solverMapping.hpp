#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::invariantgraph {
class ImplicitConstraintNode;
class InvariantNode;
class VarNode;

class SolverMapping {
  std::unordered_map<std::shared_ptr<const VarNode>, propagation::VarViewId> _solverIds{};
  std::unordered_map<std::shared_ptr<const VarNode>, propagation::VarViewId> _domainViolationIds{};
  std::unordered_map<std::shared_ptr<const InvariantNode>, std::vector<propagation::VarViewId>> _invariantIntermediateIds{};
  std::unordered_map<std::shared_ptr<const InvariantNode>, std::vector<propagation::VarViewId>> _implicitIntermediateIds{};
  std::unordered_map<std::shared_ptr<const InvariantNode>, std::shared_ptr<search::neighborhoods::Neighborhood>>
      _neighborhoods{};
  std::shared_ptr<search::neighborhoods::Neighborhood> _globalNeighborhood{
      nullptr};
  std::unordered_map<std::shared_ptr<const InvariantNode>, propagation::VarViewId> _violationIds{};
  propagation::VarViewId _totalViolationId{propagation::NULL_ID};
  propagation::VarViewId _objectiveId{propagation::NULL_ID};
  Int _objectiveOptimalValue{0};
  ObjectiveDirection _objectiveDirection{ObjectiveDirection::NONE};

  [[nodiscard]] propagation::VarViewId invariantIntermediateId(
      const std::shared_ptr<const InvariantNode>&, size_t index) const;

  [[nodiscard]] propagation::VarViewId invariantIntermediateId(
      InvariantNode&, size_t index) const;

  propagation::VarViewId setInvariantIntermediateId(
      const std::shared_ptr<const InvariantNode>&, size_t index, propagation::VarViewId solverId);

  propagation::VarViewId setInvariantIntermediateId(
      InvariantNode&, size_t index, propagation::VarViewId solverId);

  [[nodiscard]] propagation::VarViewId implicitIntermediateId(
      const std::shared_ptr<const ImplicitConstraintNode>&, size_t index) const;

  [[nodiscard]] propagation::VarViewId implicitIntermediateId(
      ImplicitConstraintNode&, size_t index) const;

  propagation::VarViewId setImplicitIntermediateId(
      const std::shared_ptr<const ImplicitConstraintNode>&, size_t index, propagation::VarViewId solverId);

  propagation::VarViewId setImplicitIntermediateId(
      ImplicitConstraintNode&, size_t index, propagation::VarViewId solverId);

 public:
  SolverMapping() = default;

  [[nodiscard]] propagation::VarViewId solverId(const std::shared_ptr<const VarNode>&) const;

  [[nodiscard]] propagation::VarViewId solverId(const VarNode&) const;

  [[nodiscard]] propagation::VarViewId domainViolationId(
      const std::shared_ptr<const VarNode>&) const;

  [[nodiscard]] propagation::VarViewId domainViolationId(
      const VarNode&) const;

  [[nodiscard]] propagation::VarViewId totalViolationId() const;

  [[nodiscard]] propagation::VarViewId objectiveId() const;

  void setSolverId(const std::shared_ptr<const VarNode>&, propagation::VarViewId solverId);

  void setSolverId(const VarNode&, propagation::VarViewId solverId);

  void setDomainViolationId(const std::shared_ptr<const VarNode>&, propagation::VarViewId solverId);

  void setDomainViolationId(const VarNode&, propagation::VarViewId solverId);

  void setTotalViolationId(propagation::VarViewId solverId);

  void setObjectiveId(propagation::VarViewId solverId);

  [[nodiscard]] bool hasNeighborhood(const std::shared_ptr<const ImplicitConstraintNode>&) const;

  [[nodiscard]] bool hasNeighborhood(const std::shared_ptr<const InvariantNode>&) const;

  [[nodiscard]] bool hasNeighborhood(const ImplicitConstraintNode&) const;
  bool hasNeighborhood(InvariantNode& invNode) const;

  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  neighborhood(const std::shared_ptr<const InvariantNode>&);

  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  neighborhood(InvariantNode&);

  [[nodiscard]] propagation::VarViewId violationId(
      const std::shared_ptr<const InvariantNode>&) const;

  [[nodiscard]] propagation::VarViewId violationId(
      const InvariantNode&) const;

  void setViolationId(const std::shared_ptr<const InvariantNode>&, propagation::VarViewId solverId);

  void setViolationId(const InvariantNode&, propagation::VarViewId solverId);

  [[nodiscard]] propagation::VarViewId intermediateId(const std::shared_ptr<const InvariantNode>&,
                                                      size_t index) const;

  [[nodiscard]] propagation::VarViewId intermediateId(const InvariantNode&,
                                                      size_t index) const;

  [[nodiscard]] propagation::VarViewId intermediateId(
      const std::shared_ptr<const InvariantNode>&) const;

  [[nodiscard]] propagation::VarViewId intermediateId(
      const InvariantNode&) const;

  propagation::VarViewId setIntermediateId(const std::shared_ptr<const InvariantNode>&, size_t index,
                                           propagation::VarViewId solverId);

  propagation::VarViewId setIntermediateId(const InvariantNode&, size_t index,
                                           propagation::VarViewId solverId);

  propagation::VarViewId setIntermediateId(const std::shared_ptr<const InvariantNode>&,
                                           propagation::VarViewId solverId);

  propagation::VarViewId setIntermediateId(const InvariantNode&,
                                           propagation::VarViewId solverId);

  [[nodiscard]] bool hasGlobalNeighborhood() const;

  [[nodiscard]] std::shared_ptr<search::neighborhoods::Neighborhood>
  globalNeighborhood();

  void setGlobalNeighborhood(
      const std::shared_ptr<search::neighborhoods::Neighborhood>&);

  bool setNeighborhood(
      const std::shared_ptr<const InvariantNode>&,
      const std::shared_ptr<search::neighborhoods::Neighborhood>&);

  bool setNeighborhood(
      const InvariantNode&,
      const std::shared_ptr<search::neighborhoods::Neighborhood>&);

  [[nodiscard]] Int objectiveOptimalValue() const;

  void setObjectiveOptimalValue(Int value);

  [[nodiscard]] ObjectiveDirection objectiveDirection() const;

  void setObjectiveDirection(ObjectiveDirection);
};

}  // namespace atlantis::invariantgraph