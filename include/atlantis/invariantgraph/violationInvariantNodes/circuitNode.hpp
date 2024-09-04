#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class CircuitNode : public ViolationInvariantNode {
 public:
  explicit CircuitNode(IInvariantGraph& graph, std::vector<VarNodeId>&&);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
};
}  // namespace atlantis::invariantgraph
