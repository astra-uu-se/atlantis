#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
 private:
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolAndNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                   VarNodeId output);

  ArrayBoolAndNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                   bool shouldHold = true);

  ArrayBoolAndNode(IInvariantGraph& graph, std::vector<VarNodeId>&& as,
                   VarNodeId output);

  ArrayBoolAndNode(IInvariantGraph& graph, std::vector<VarNodeId>&& as,
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
