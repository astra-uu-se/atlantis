#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class BoolClauseNode : public ViolationInvariantNode {
 private:
  size_t _numAs;
  propagation::VarId _sumVarId{propagation::NULL_ID};

 public:
  explicit BoolClauseNode(InvariantGraph& graph, std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, VarNodeId r);

  explicit BoolClauseNode(InvariantGraph& graph, std::vector<VarNodeId>&& as,
                          std::vector<VarNodeId>&& bs, bool shouldHold = true);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
