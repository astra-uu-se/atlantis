#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
 private:
  Int _ub;

 public:
  explicit ArrayIntMinimumNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                               VarNodeId output);

  explicit ArrayIntMinimumNode(IInvariantGraph& graph,

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
