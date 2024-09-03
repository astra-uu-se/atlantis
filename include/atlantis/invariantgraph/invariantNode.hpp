#pragma once

#include <vector>

#include "atlantis/invariantgraph/iInvariantNode.hpp"
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

class InvariantNode : public IInvariantNode {
 private:
  InvariantNodeId _id{NULL_NODE_ID};
  InvariantNodeState _state{InvariantNodeState::UNINITIALIZED};
  InvariantGraph& _invariantGraph;

 protected:
  std::vector<VarNodeId> _outputVarNodeIds;
  std::vector<VarNodeId> _staticInputVarNodeIds;
  std::vector<VarNodeId> _dynamicInputVarNodeIds;

 public:
  explicit InvariantNode(InvariantGraph& invariantGraph,
                         std::vector<VarNodeId>&& outputIds,
                         std::vector<VarNodeId>&& staticInputIds = {},
                         std::vector<VarNodeId>&& dynamicInputIds = {});

  virtual ~InvariantNode() = default;

  InvariantGraph& invariantGraph();

  const InvariantGraph& invariantGraphConst() const;

  propagation::SolverBase& solver();

  const propagation::SolverBase& solverConst() const;

  InvariantNodeId id() const override;

  InvariantNodeState state() const override;

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

  std::vector<std::pair<VarNodeId, VarNodeId>> splitOutputVarNodes() override;

  propagation::VarId makeSolverVar(VarNodeId varNodeId) override;

  propagation::VarId makeSolverVar(VarNodeId varNodeId,
                                   Int initialValue) override;

  void markOutputTo(VarNodeId varNodeId, bool registerHere) override;

  void markStaticInputTo(VarNodeId varNodeId, bool registerHere) override;

  void markDynamicInputTo(VarNodeId varNodeId, bool registerHere) override;
};

}  // namespace atlantis::invariantgraph
