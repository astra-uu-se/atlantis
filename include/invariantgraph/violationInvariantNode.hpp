#pragma once

#include <cassert>
#include <vector>

#include "core/engine.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariantgraph/varNode.hpp"

namespace invariantgraph {

class InvariantGraph;  // Forward declaration

/**
 * The types that can be in an array of search variables.
 */
using MappableValue = std::variant<Int, bool, std::string_view>;

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation engine.
 */
class ViolationInvariantNode : public InvariantNode {
 private:
  // Bounds will be recomputed by the engine.
  VarId _violationVarId{NULL_ID};
  VarNodeId _reifiedViolation;

  // If the constraint is not reified, then this boolean indicates if the
  // constraint should hold or not:
  const bool _shouldHold;

  explicit ViolationInvariantNode(
      std::vector<VarNodeId>&& definedVars,
      std::vector<VarNodeId>&& staticInputVarNodeIds,
      VarNodeId reifiedViolation, bool shouldHold);

 protected:
  inline bool shouldHold() const noexcept;

 public:
  explicit ViolationInvariantNode(
      std::vector<VarNodeId>&& outputVarNodeIds,
      std::vector<VarNodeId>&& staticInputVarNodeIds,
      VarNodeId reifiedViolation);

  explicit ViolationInvariantNode(
      std::vector<VarNodeId>&& staticInputVarNodeIds,
      VarNodeId reifiedViolation);

  /**
   * @brief Construct a new Soft Constraint Node object
   *
   * @param staticInputVarNodeIds
   * @param shouldHold true if the constraint should hold, else false
   */
  explicit ViolationInvariantNode(
      std::vector<VarNodeId>&& outputVarNodeIds,
      std::vector<VarNodeId>&& staticInputVarNodeIds, bool shouldHold);

  explicit ViolationInvariantNode(
      std::vector<VarNodeId>&& staticInputVarNodeIds, bool shouldHold);

  ~ViolationInvariantNode() override = default;

  [[nodiscard]] bool isReified() const override;

  [[nodiscard]] VarId violationVarId() const override;

  inline VarNodeId reifiedViolation();

 protected:
  VarId setViolationVarId(VarId varId);

  inline VarId registerViolation(Engine& engine, Int initialValue = 0);
};

}  // namespace invariantgraph