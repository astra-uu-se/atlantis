#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
 private:
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolXorNode(IInvariantGraph& graph, VarNodeId aVarNodeId,
                   VarNodeId bNodeId, VarNodeId reifiedVarNodeId);

  ArrayBoolXorNode(IInvariantGraph& graph, VarNodeId aVarNodeId,
                   VarNodeId bNodeId, bool shouldHold = true);

  ArrayBoolXorNode(IInvariantGraph& graph,
                   std::vector<VarNodeId>&& inputVarNodeIds,
                   VarNodeId reifiedVarNodeId);

  ArrayBoolXorNode(IInvariantGraph& graph,
                   std::vector<VarNodeId>&& inputVarNodeIds,
                   bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
