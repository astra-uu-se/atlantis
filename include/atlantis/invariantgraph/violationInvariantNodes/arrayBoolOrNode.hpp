#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolOrNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolOrNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                  VarNodeId output);

  ArrayBoolOrNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                  bool shouldHold = true);

  ArrayBoolOrNode(IInvariantGraph& graph, std::vector<VarNodeId>&& inputs,
                  VarNodeId output);

  ArrayBoolOrNode(IInvariantGraph& graph, std::vector<VarNodeId>&& inputs,
                  bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};

}  // namespace atlantis::invariantgraph
