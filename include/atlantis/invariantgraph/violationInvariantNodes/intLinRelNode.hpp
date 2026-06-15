#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class IntLinRelNode : public ViolationInvariantNode {
  RelationType _relType;
  std::vector<Int> _coeffs;
  Int _rhs;

  void updateRelType();

 public:
  IntLinRelNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, RelationType relationType,
                Int rhs, bool shouldHold = true);

  IntLinRelNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, RelationType relType, Int bound,
                VarNodeId reified);

  void init(InvariantNodeId) override;

  void postConstraint() override;

  void updateState() override;

  void registerOutputVars(propagation::SolverBase&,
                          SolverMapping&) const override;

  void registerNode(propagation::SolverBase&, SolverMapping&) const override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;

  [[nodiscard]] std::string dotLangIdentifier() const override;
};

}  // namespace atlantis::invariantgraph
