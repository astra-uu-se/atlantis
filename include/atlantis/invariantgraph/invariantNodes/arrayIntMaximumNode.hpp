#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
  Int _lb;

 public:
  explicit ArrayIntMaximumNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                               VarNodeId output);

  explicit ArrayIntMaximumNode(InvariantGraph& graph,

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
