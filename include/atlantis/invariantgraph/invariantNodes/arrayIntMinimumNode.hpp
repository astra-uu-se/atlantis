#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
 private:
  Int _ub;

 public:
  explicit ArrayIntMinimumNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                               VarNodeId output);

  explicit ArrayIntMinimumNode(InvariantGraph& graph,

                               std::vector<VarNodeId>&& vars, VarNodeId output);

  void init(const InvariantNodeId&) override;

  void registerOutputVars() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
