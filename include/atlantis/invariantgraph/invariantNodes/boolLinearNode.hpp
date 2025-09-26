#pragma once

#include "atlantis/invariantgraph/invariantNode.hpp"

namespace atlantis::invariantgraph {
class BoolLinearNode : public InvariantNode {
  std::vector<Int> _coeffs;
  Int _offset{0};

 public:
  BoolLinearNode(InvariantGraph& graph,

                 std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                 VarNodeId output, Int offset = 0);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};
}  // namespace atlantis::invariantgraph
