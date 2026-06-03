#pragma once

#include <optional>

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class CountNode : public InvariantNode {
  std::optional<Int> _fixedNeedle;
  Int _offset;

  [[nodiscard]] VarNodeId needle() const;
  [[nodiscard]] size_t needleIndex() const;
  [[nodiscard]] size_t numInputVars() const;

 public:
  CountNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars, Int needle,
            VarNodeId count, Int offset = 0);

  CountNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
            VarNodeId needle, VarNodeId count, Int offset = 0);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
