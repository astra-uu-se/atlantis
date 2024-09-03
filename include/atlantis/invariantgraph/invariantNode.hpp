#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantNodeBase.hpp"
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

class InvariantNode : public InvariantNodeBase {
 private:
  InvariantGraph& _invariantGraph;

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

  virtual void init(InvariantNodeId) override;

  virtual void deactivate() override;

  void replaceDefinedVar(VarNodeId oldOutputVarNodeId,
                         VarNodeId newOutputVarNodeId) override;

  void removeStaticInputVarNode(VarNodeId) override;

  void removeDynamicInputVarNode(VarNodeId) override;

  void removeOutputVarNode(VarNodeId) override;

  void replaceStaticInputVarNode(VarNodeId oldInputVarNodeId,
                                 VarNodeId newInputVarNodeId) override;

  void replaceDynamicInputVarNode(VarNodeId oldInputVarNodeId,
                                  VarNodeId newInputVarNodeId) override;

  std::vector<std::pair<VarNodeId, VarNodeId>> splitOutputVarNodes() override;

  virtual propagation::VarId makeSolverVar(VarNodeId varNodeId) override;

  virtual propagation::VarId makeSolverVar(VarNodeId varNodeId,
                                           Int initialValue) override;

  void markOutputTo(VarNodeId varNodeId, bool registerHere) override;

  void markStaticInputTo(VarNodeId varNodeId, bool registerHere) override;

  void markDynamicInputTo(VarNodeId varNodeId, bool registerHere) override;
};

}  // namespace atlantis::invariantgraph
