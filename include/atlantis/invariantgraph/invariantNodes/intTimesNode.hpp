#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntTimesNode : public InvariantNode {
 private:
  Int _scalar{1};
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  IntTimesNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
               VarNodeId output);

  void init(const InvariantNodeId&) override;

  void updateState() override;

  bool canBeReplaced() const override;

  bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};

}  // namespace atlantis::invariantgraph
