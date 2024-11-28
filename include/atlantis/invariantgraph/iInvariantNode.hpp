#pragma once

#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a violation invariant (which defines a violation), or a
 * view.
 */

class IInvariantNode {
 public:
  virtual ~IInvariantNode() = default;

  virtual void init(InvariantNodeId) = 0;

  [[nodiscard]] virtual InvariantNodeId id() const = 0;

  [[nodiscard]] virtual bool isReified() const = 0;

  [[nodiscard]] virtual bool canBeReplaced() const = 0;

  [[nodiscard]] virtual bool replace() = 0;

  [[nodiscard]] virtual bool canBeMadeImplicit() const = 0;

  [[nodiscard]] virtual bool makeImplicit() = 0;

  [[nodiscard]] virtual InvariantNodeState state() const = 0;

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
  [[nodiscard]] virtual const std::vector<VarNodeId>& outputVarNodeIds()
      const = 0;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] virtual propagation::VarViewId violationVarId() const = 0;

  [[nodiscard]] virtual const std::vector<VarNodeId>& staticInputVarNodeIds()
      const = 0;

  [[nodiscard]] virtual const std::vector<VarNodeId>& dynamicInputVarNodeIds()
      const = 0;

  virtual void updateState() = 0;

  virtual void replaceDefinedVar(VarNodeId oldOutputVarNode,
                                 VarNodeId newOutputVarNode) = 0;

  virtual void removeStaticInputVarNode(VarNodeId) = 0;

  virtual void removeDynamicInputVarNode(VarNodeId) = 0;

  virtual void removeOutputVarNode(VarNodeId) = 0;

  virtual void replaceStaticInputVarNode(VarNodeId oldInputVarNode,
                                         VarNodeId newInputVarNode) = 0;

  virtual void replaceDynamicInputVarNode(VarNodeId oldInputVarNode,
                                          VarNodeId newInputVarNode) = 0;

  /**
   * Splits the unfixed output variable nodes of the current node into multiple
   * nodes.
   * @return A vector where an element (i, j) means that VarNode with VarNodeId
   * i has been replaced by the VarNode with VarNodeId j.
   */
  virtual std::vector<std::pair<VarNodeId, VarNodeId>>
  splitOutputVarNodes() = 0;

  virtual propagation::VarViewId makeSolverVar(VarNodeId) = 0;

  virtual propagation::VarViewId makeSolverVar(VarNodeId, Int initialValue) = 0;

  virtual void markOutputTo(VarNodeId node, bool registerHere) = 0;

  virtual void markStaticInputTo(VarNodeId node, bool registerHere) = 0;

  virtual void markDynamicInputTo(VarNodeId node, bool registerHere) = 0;

  virtual void setState(InvariantNodeState) = 0;

  virtual std::string dotLangIdentifier() const = 0;

  virtual std::ostream& dotLangEdges(std::ostream&) const = 0;

  virtual std::ostream& dotLangEntry(std::ostream&) const = 0;
};
}  // namespace atlantis::invariantgraph
