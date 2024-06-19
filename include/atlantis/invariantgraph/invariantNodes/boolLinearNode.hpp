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
  propagation::VarId _intermediate{propagation::NULL_ID};
  Int _offset{0};

 public:
  BoolLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                 VarNodeId output);

  void updateState(InvariantGraph& graph) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};
}  // namespace atlantis::invariantgraph
