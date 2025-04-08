#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {
class CircuitNode : public ViolationInvariantNode {
  Int _offset;

 public:
  explicit CircuitNode(InvariantGraph& graph, std::vector<VarNodeId>&&,
                       Int offset);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;
  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
