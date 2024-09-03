#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolOrNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                  VarNodeId output);

  ArrayBoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                  bool shouldHold = true);

  ArrayBoolOrNode(InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
                  VarNodeId output);

  ArrayBoolOrNode(InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
                  bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};

}  // namespace atlantis::invariantgraph
