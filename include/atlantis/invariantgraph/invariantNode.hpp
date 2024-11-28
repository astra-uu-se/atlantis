#pragma once

#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/invariantgraph/iInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a violation invariant (which defines a violation), or a
 * view.
 */

class InvariantNode : virtual public IInvariantNode {
 private:
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

  virtual ~InvariantNode() = default;

  [[nodiscard]] IInvariantGraph& invariantGraph();

  [[nodiscard]] const IInvariantGraph& invariantGraphConst() const;

  [[nodiscard]] propagation::SolverBase& solver();

  [[nodiscard]] const propagation::SolverBase& solverConst() const;

  [[nodiscard]] InvariantNodeId id() const override;

  [[nodiscard]] virtual bool isReified() const override;

  virtual void updateState() override;

  [[nodiscard]] virtual bool canBeReplaced() const override;

  [[nodiscard]] virtual bool replace() override;

  [[nodiscard]] virtual bool canBeMadeImplicit() const override;

  [[nodiscard]] virtual bool makeImplicit() override;

  [[nodiscard]] InvariantNodeState state() const override;

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a violation invariant. If this node does
   * not define a violation variable, this method returns propagation::NULL_ID.
   */
  [[nodiscard]] propagation::VarViewId violationVarId() const;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VarNodeId>& outputVarNodeIds() const;

  [[nodiscard]] const std::vector<VarNodeId>& staticInputVarNodeIds() const;

  [[nodiscard]] const std::vector<VarNodeId>& dynamicInputVarNodeIds() const;

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

  [[nodiscard]] propagation::VarViewId makeSolverVar(
      VarNodeId varNodeId) override;

  [[nodiscard]] propagation::VarViewId makeSolverVar(VarNodeId varNodeId,
                                                     Int initialValue) override;

  void markOutputTo(VarNodeId varNodeId, bool registerHere) override;

  void markStaticInputTo(VarNodeId varNodeId, bool registerHere) override;

  void markDynamicInputTo(VarNodeId varNodeId, bool registerHere) override;

  virtual std::ostream& dotLangEdges(std::ostream&) const override;

  virtual std::ostream& dotLangEntry(std::ostream&) const override;
};

}  // namespace atlantis::invariantgraph
