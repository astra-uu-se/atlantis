#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class ArrayBoolXorNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  ArrayBoolXorNode(InvariantGraph& graph,

                   VarNodeId aVarNodeId, VarNodeId bNodeId,
                   VarNodeId reifiedVarNodeId);

  ArrayBoolXorNode(InvariantGraph& graph,

                   VarNodeId aVarNodeId, VarNodeId bNodeId,
                   bool shouldHold = true);

  ArrayBoolXorNode(InvariantGraph& graph,

                   std::vector<VarNodeId>&& inputVarNodeIds,
                   VarNodeId reifiedVarNodeId);

  ArrayBoolXorNode(InvariantGraph& graph,

                   std::vector<VarNodeId>&& inputVarNodeIds,
                   bool shouldHold = true);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};

}  // namespace atlantis::invariantgraph
