#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolAndNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                   VarNodeId output);

  ArrayBoolAndNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                   bool shouldHold = true);

  ArrayBoolAndNode(InvariantGraph& graph, std::vector<VarNodeId>&& as,
                   VarNodeId output);

  ArrayBoolAndNode(InvariantGraph& graph, std::vector<VarNodeId>&& as,
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
