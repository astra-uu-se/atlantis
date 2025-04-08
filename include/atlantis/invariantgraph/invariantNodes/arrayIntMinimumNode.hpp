#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
  Int _ub;

 public:
  explicit ArrayIntMinimumNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                               VarNodeId output);

  explicit ArrayIntMinimumNode(InvariantGraph& graph,

                               std::vector<VarNodeId>&& vars, VarNodeId output);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerNode() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
