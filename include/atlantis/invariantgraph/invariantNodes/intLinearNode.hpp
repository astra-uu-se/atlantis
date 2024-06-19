#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  propagation::VarId _intermediate{propagation::NULL_ID};
  Int _offset{0};

  IntLinearNode(
      std::pair<std::vector<Int>, std::vector<VarNodeId>>&& coeffsAndVars,
      VarNodeId output);

 public:
  IntLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                VarNodeId output);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  bool replace(InvariantGraph& graph) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};
}  // namespace atlantis::invariantgraph
