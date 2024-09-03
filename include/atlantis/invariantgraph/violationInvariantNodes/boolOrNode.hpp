#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class BoolOrNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  BoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b, VarNodeId r);

  BoolOrNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
             bool shouldHold = true);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId a() const noexcept {
    return staticInputVarNodeIds().front();
  }
  [[nodiscard]] VarNodeId b() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
