#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class BoolAllEqualNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit BoolAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                            VarNodeId r, bool breaksCycle = false);

  explicit BoolAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                            bool shouldHold = true, bool breaksCycle = false);

  explicit BoolAllEqualNode(InvariantGraph& graph,
                            std::vector<VarNodeId>&& vars, VarNodeId r,
                            bool breaksCycle = false);

  explicit BoolAllEqualNode(InvariantGraph& graph,
                            std::vector<VarNodeId>&& vars,
                            bool shouldHold = true, bool breaksCycle = false);

  void init(InvariantNodeId) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
