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

class InvariantNode {
 private:
  std::vector<VarNodeId> _outputVarNodeIds;
  std::vector<VarNodeId> _staticInputVarNodeIds;
  std::vector<VarNodeId> _dynamicInputVarNodeIds;
  InvariantNodeId _id{NULL_NODE_ID};
  InvariantNodeState _state{InvariantNodeState::UNINITIALIZED};

 public:
  explicit InvariantNode(std::vector<VarNodeId>&& outputIds,
                         std::vector<VarNodeId>&& staticInputIds = {},
                         std::vector<VarNodeId>&& dynamicInputIds = {});

  virtual ~InvariantNode() = default;

  void init(InvariantGraph&, const InvariantNodeId&);

  [[nodiscard]] InvariantNodeId id() const { return _id; }

  [[nodiscard]] virtual bool isReified() const;

  [[nodiscard]] virtual bool canBeReplaced(const InvariantGraph&) const;

  [[nodiscard]] virtual bool replace(InvariantGraph&);

  [[nodiscard]] InvariantNodeState state() const { return _state; }

  void deactivate(InvariantGraph&);

  /**
   * Creates as all the variables the node defines in @p solver.
   *
   * @param solver The solver with which to register the variables, constraints
   * and views.
   */
  virtual void registerOutputVars(InvariantGraph&,
                                  propagation::SolverBase&) = 0;

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
  virtual void registerNode(InvariantGraph&, propagation::SolverBase&) = 0;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VarNodeId>& outputVarNodeIds() const noexcept;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] virtual propagation::VarId violationVarId(
      const InvariantGraph&) const;

  [[nodiscard]] const std::vector<VarNodeId>& staticInputVarNodeIds()
      const noexcept;

  [[nodiscard]] const std::vector<VarNodeId>& dynamicInputVarNodeIds()
      const noexcept;

  virtual void updateVariableNodes(InvariantGraph&){};

  void replaceDefinedVar(VarNode& oldOutputVarNode, VarNode& newOutputVarNode);

  void removeStaticInputVarNode(VarNode&);

  void removeDynamicInputVarNode(VarNode&);

  void removeOutputVarNode(VarNode&);

  void replaceStaticInputVarNode(VarNode& oldInputVarNode,
                                 VarNode& newInputVarNode);

  void replaceDynamicInputVarNode(VarNode& oldInputVarNode,
                                  VarNode& newInputVarNode);

  // A hack in order to steal the _inputs from the nested constraint.
  friend class ReifiedConstraint;

 protected:
  static propagation::VarId makeSolverVar(propagation::SolverBase&, VarNode&,
                                          Int initialValue = 0);

  void markOutputTo(VarNode& node, bool registerHere = true);

  void markStaticInputTo(VarNode& node, bool registerHere = true);

  void markDynamicInputTo(VarNode& node, bool registerHere = true);

  void setState(InvariantNodeState state) { _state = state; }
};
}  // namespace atlantis::invariantgraph
