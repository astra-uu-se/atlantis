#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class IntCountNode : public InvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};
  Int _offset{0};
  Int _needle;

 public:
  IntCountNode(InvariantGraph& graph,

               std::vector<VarNodeId>&& vars, Int needle, VarNodeId count);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<VarNodeId>& haystack() const;

  [[nodiscard]] Int needle() const;
};
}  // namespace atlantis::invariantgraph
