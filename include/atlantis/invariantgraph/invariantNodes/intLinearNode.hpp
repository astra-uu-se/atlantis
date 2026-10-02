#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntLinearNode : public InvariantNode {
  std::vector<Int> _coeffs;
  Int _rhsOffset;

 public:
  IntLinearNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<std::shared_ptr<VarNode>>&& vars, VarNode& output,
                Int rhsOffset = 0);

  void init() override;

  void postConstraint() override;

  void updateState() override;

  [[nodiscard]] bool constrainsOutput(
      VarNode& outputVarNode) const override;

  [[nodiscard]] std::pair<size_t, size_t> implicitRank() const override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
