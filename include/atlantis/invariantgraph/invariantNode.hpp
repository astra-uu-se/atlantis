#pragma once

#include "atlantis/invariantgraph/iInvariantNode.hpp"

namespace atlantis::propagation {
class SolverBase;  //  forward declaration;
}

namespace atlantis::invariantgraph {

class IInvariantGraph;  // forward declaration;

/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a violation invariant (which defines a violation), or a
 * view.
 */

class InvariantNode : virtual public IInvariantNode {
  InvariantNodeId _id{NULL_NODE_ID};
  InvariantNodeState _state{InvariantNodeState::UNINITIALIZED};
  IInvariantGraph& _invariantGraph;

 protected:
  std::vector<VarNodeId> _outputVarNodeIds;
  std::vector<VarNodeId> _staticInputVarNodeIds;
  std::vector<VarNodeId> _dynamicInputVarNodeIds;

 public:
  explicit InvariantNode(IInvariantGraph& invariantGraph,
                         std::vector<VarNodeId>&& outputIds,
                         std::vector<VarNodeId>&& staticInputIds = {},
                         std::vector<VarNodeId>&& dynamicInputIds = {});

  [[nodiscard]] IInvariantGraph& invariantGraph();

  [[nodiscard]] const IInvariantGraph& invariantGraphConst() const;

  [[nodiscard]] propagation::SolverBase& solver();

  [[nodiscard]] const propagation::SolverBase& solverConst() const;

  [[nodiscard]] InvariantNodeId id() const override;

  [[nodiscard]] bool isReified() const override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] InvariantNodeState state() const override;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] propagation::VarViewId violationVarId() const override;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VarNodeId>& outputVarNodeIds() const override;

  [[nodiscard]] const std::vector<VarNodeId>& staticInputVarNodeIds()
      const override;

  [[nodiscard]] const std::vector<VarNodeId>& dynamicInputVarNodeIds()
      const override;

  void setState(InvariantNodeState) override;

  void init(InvariantNodeId) override;

  void deactivate() override;

  void replaceDefinedVar(VarNodeId oldOutputVarNodeId,
                         VarNodeId newOutputVarNodeId) override;

  void removeStaticInputVarNode(VarNodeId) override;

  void removeDynamicInputVarNode(VarNodeId) override;

  void removeOutputVarNode(VarNodeId) override;

  void eraseStaticInputVarNode(size_t index);

  void eraseDynamicInputVarNode(size_t index);

  void replaceStaticInputVarNode(VarNodeId oldInputVarNodeId,
                                 VarNodeId newInputVarNodeId) override;

  void replaceDynamicInputVarNode(VarNodeId oldInputVarNodeId,
                                  VarNodeId newInputVarNodeId) override;

  [[nodiscard]] std::vector<std::pair<VarNodeId, VarNodeId>>
  splitOutputVarNodes() override;

  propagation::VarViewId makeSolverVar(VarNodeId varNodeId) override;

  propagation::VarViewId makeSolverVar(VarNodeId varNodeId,
                                       Int initialValue) override;

  void markOutputTo(VarNodeId varNodeId, bool registerHere) override;

  void markStaticInputTo(VarNodeId varNodeId, bool registerHere) override;

  void markDynamicInputTo(VarNodeId varNodeId, bool registerHere) override;

  std::ostream& dotLangEdges(std::ostream&) const override;

  std::ostream& dotLangEntry(std::ostream&) const override;
};

}  // namespace atlantis::invariantgraph
