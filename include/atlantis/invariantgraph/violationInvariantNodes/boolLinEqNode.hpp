#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class BoolLinEqNode : public ViolationInvariantNode {
  std::vector<Int> _coeffs;
  Int _bound;

 public:
  BoolLinEqNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, Int bound,
                bool shouldHold = true);

  BoolLinEqNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, Int bound, VarNodeId reified);

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
