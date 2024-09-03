#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolAndNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

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

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};

}  // namespace atlantis::invariantgraph
