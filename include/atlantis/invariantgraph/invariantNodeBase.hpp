#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a violation invariant (which defines a violation), or a
 * view.
 */

class InvariantGraph;  // Forward declaration

class InvariantNodeBase {
 private:
  InvariantNodeId _id{NULL_NODE_ID};
  InvariantNodeState _state{InvariantNodeState::UNINITIALIZED};

 public:
  explicit InvariantNodeBase(std::vector<VarNodeId>&& outputIds,
                             std::vector<VarNodeId>&& staticInputIds = {},
                             std::vector<VarNodeId>&& dynamicInputIds = {});

  virtual ~InvariantNodeBase() = default;

  virtual void init(const InvariantNodeId&);

  [[nodiscard]] InvariantNodeId id() const { return _id; }

  [[nodiscard]] virtual bool isReified() const;

  [[nodiscard]] virtual bool canBeReplaced() const;

  [[nodiscard]] virtual bool replace();

  [[nodiscard]] virtual bool canBeMadeImplicit() const;

  [[nodiscard]] virtual bool makeImplicit();

  [[nodiscard]] InvariantNodeState state() const { return _state; }

  virtual void deactivate() = 0;

  /**
   * Creates as all the variables the node defines in @p solver.
   *
   * @param solver The solver with which to register the variables, constraints
   * and views.
   */
  virtual void registerOutputVars() = 0;

  /**
   * Registers the current node with the solver, as well as all the variables
   * it defines.
   *
   * Note: This method assumes it is called after all the inputs to this node
   * are already registered with the solver.
   *
   * @param solver The solver with which to register the variables, constraints
   * and views.
   */
  virtual void registerNode() = 0;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VarNodeId>& outputVarNodeIds() const noexcept;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] virtual propagation::VarId violationVarId() const;

  [[nodiscard]] const std::vector<VarNodeId>& staticInputVarNodeIds()
      const noexcept;

  [[nodiscard]] const std::vector<VarNodeId>& dynamicInputVarNodeIds()
      const noexcept;

  virtual void updateState(){};

  virtual void replaceDefinedVar(VarNodeId oldOutputVarNode,
                                 VarNodeId newOutputVarNode) = 0;

  virtual void removeStaticInputVarNode(VarNodeId) = 0;

  virtual void removeDynamicInputVarNode(VarNodeId) = 0;

  virtual void removeOutputVarNode(VarNodeId) = 0;

  virtual void replaceStaticInputVarNode(VarNodeId oldInputVarNode,
                                         VarNodeId newInputVarNode) = 0;

  virtual void replaceDynamicInputVarNode(VarNodeId oldInputVarNode,
                                          VarNodeId newInputVarNode) = 0;

  // A hack in order to steal the _inputs from the nested constraint.
  friend class ReifiedConstraint;

 protected:
  std::vector<VarNodeId> _outputVarNodeIds;
  std::vector<VarNodeId> _staticInputVarNodeIds;
  std::vector<VarNodeId> _dynamicInputVarNodeIds;

  void eraseStaticInputVarNode(size_t index);

  void eraseDynamicInputVarNode(size_t index);

  /**
   * Splits the unfixed output variable nodes of the current node into multiple
   * nodes.
   * @return A vector where an element (i, j) means that VarNode with VarNodeId
   * i has been replaced by the VarNode with VarNodeId j.
   */
  virtual std::vector<std::pair<VarNodeId, VarNodeId>>
  splitOutputVarNodes() = 0;

  virtual propagation::VarId makeSolverVar(VarNodeId) = 0;

  virtual propagation::VarId makeSolverVar(VarNodeId, Int initialValue) = 0;

  virtual void markOutputTo(VarNodeId node, bool registerHere) = 0;

  virtual void markStaticInputTo(VarNodeId node, bool registerHere) = 0;

  virtual void markDynamicInputTo(VarNodeId node, bool registerHere) = 0;

  void setState(InvariantNodeState state) { _state = state; }
};
}  // namespace atlantis::invariantgraph
