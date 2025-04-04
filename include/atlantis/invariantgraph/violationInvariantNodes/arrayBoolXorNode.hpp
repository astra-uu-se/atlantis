#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolXorNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                   VarNodeId reified);

  ArrayBoolXorNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                   bool shouldHold = true);

  ArrayBoolXorNode(IInvariantGraph& graph, std::vector<VarNodeId>&& inputVars,
                   VarNodeId reified);

  ArrayBoolXorNode(IInvariantGraph& graph,
                   std::vector<VarNodeId>&& inputVarNodeIds,
                   bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
