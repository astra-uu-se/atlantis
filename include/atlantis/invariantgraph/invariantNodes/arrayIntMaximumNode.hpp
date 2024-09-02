#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
 private:
  Int _lb;

 public:
  explicit ArrayIntMaximumNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                               VarNodeId output);

  explicit ArrayIntMaximumNode(IInvariantGraph& graph,

                               std::vector<VarNodeId>&& vars, VarNodeId output);

  void init(InvariantNodeId) override;

  void registerOutputVars() override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerNode() override;
  virtual std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
