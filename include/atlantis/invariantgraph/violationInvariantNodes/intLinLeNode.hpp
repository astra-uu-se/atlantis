#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntLinLeNode : public ViolationInvariantNode {
  std::vector<Int> _coeffs;
  Int _bound;


 public:
  IntLinLeNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
               std::vector<VarNodeId>&& vars, Int bound,
               bool shouldHold = true);

  IntLinLeNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
               std::vector<VarNodeId>&& vars, Int bound, VarNodeId reified);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&, SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
