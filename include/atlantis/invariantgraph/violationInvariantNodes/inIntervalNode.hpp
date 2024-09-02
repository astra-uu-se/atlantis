#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class InIntervalNode : public ViolationInvariantNode {
 private:
  Int _lb, _ub;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit InIntervalNode(InvariantGraph& graph,

                          VarNodeId input, Int lb, Int ub, VarNodeId r);

  explicit InIntervalNode(InvariantGraph& graph,

                          VarNodeId input, Int lb, Int ub,
                          bool shouldHold = true);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
