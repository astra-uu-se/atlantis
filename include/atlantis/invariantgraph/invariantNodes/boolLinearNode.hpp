#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {
class BoolLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset{0};
  propagation::VarId _intermediate{propagation::NULL_ID};

  BoolLinearNode(
      std::pair<std::vector<Int>, std::vector<VarNodeId>>&& coeffsAndVars,
      VarNodeId output, Int offset = 0);

 public:
  BoolLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                 VarNodeId output, Int offset = 0);

  void updateState(InvariantGraph& graph) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};
}  // namespace atlantis::invariantgraph
