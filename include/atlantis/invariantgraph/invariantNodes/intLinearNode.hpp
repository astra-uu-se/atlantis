#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {

class IntLinearNode : public InvariantNode {
  std::vector<Int> _coeffs;
  Int _offset;

 public:
  IntLinearNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, VarNodeId output,
                Int offset = 0);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeMadeImplicit() const override;

  [[nodiscard]] bool makeImplicit() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
