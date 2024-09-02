#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {

class IntAllEqualNode : public ViolationInvariantNode {
 private:
  bool _breaksCycle{false};
  propagation::VarId _allDifferentViolationVarId{propagation::NULL_ID};

 public:
  explicit IntAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                           VarNodeId r, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                           bool shouldHold = true, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           VarNodeId r, bool breaksCycle = false);

  explicit IntAllEqualNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           bool shouldHold = true, bool breaksCycle = false);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
