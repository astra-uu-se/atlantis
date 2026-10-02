#pragma once

#include <optional>

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class CountNode : public InvariantNode {
  std::optional<Int> _fixedNeedle;
  Int _countOffset;

  [[nodiscard]] std::shared_ptr<VarNode> needle() const;
  [[nodiscard]] size_t needleIndex() const;
  [[nodiscard]] size_t numInputVars() const;

 public:
  CountNode(InvariantGraph& graph, VarNode& count,
            std::vector<std::shared_ptr<VarNode>>&& vars, Int needle,
            Int countOffset = 0);

  CountNode(InvariantGraph& graph, VarNode& count,
            std::vector<std::shared_ptr<VarNode>>&& vars, VarNode& needle,
            Int countOffset = 0);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      const VarNode& outputVarNode) const override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] std::pair<size_t, size_t> implicitRank() const override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  bool makeImplicit() override;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
