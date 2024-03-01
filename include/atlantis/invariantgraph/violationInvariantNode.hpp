#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation solver.
 */
class ViolationInvariantNode : public InvariantNode {
 private:
  // Bounds will be recomputed by the solver.
  propagation::VarId _violationVarId{propagation::NULL_ID};
  VarNodeId _reifiedViolationNodeId;

  // If the violation invariant is not reified, then this boolean indicates if
  // the violation invariant should hold or not:
  const bool _shouldHold;

  explicit ViolationInvariantNode(std::vector<VarNodeId>&& outputIds,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  VarNodeId reifiedViolationId,
                                  bool shouldHold);

 protected:
  [[nodiscard]] bool shouldHold() const noexcept;

 public:
  explicit ViolationInvariantNode(std::vector<VarNodeId>&& outputIds,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  VarNodeId reifiedViolationId);

  explicit ViolationInvariantNode(std::vector<VarNodeId>&& staticInputIds,
                                  VarNodeId reifiedViolationId);

  /**
   * @brief Construct a new violation invariant node object
   *
   * @param staticInputVarNodeIds
   * @param shouldHold true if the violation invariant should hold, else false
   */
  explicit ViolationInvariantNode(std::vector<VarNodeId>&& outputIds,
                                  std::vector<VarNodeId>&& staticInputIds,
                                  bool shouldHold);

  explicit ViolationInvariantNode(std::vector<VarNodeId>&& staticInputIds,
                                  bool shouldHold);

  ~ViolationInvariantNode() override = default;

  [[nodiscard]] bool isReified() const override;

  [[nodiscard]] propagation::VarId violationVarId(
      const InvariantGraph&) const override;

  VarNodeId reifiedViolationNodeId();

 protected:
  propagation::VarId setViolationVarId(InvariantGraph&, propagation::VarId);

  propagation::VarId registerViolation(InvariantGraph&,
                                       propagation::SolverBase&,
                                       Int initialValue = 0);
};

}  // namespace atlantis::invariantgraph
