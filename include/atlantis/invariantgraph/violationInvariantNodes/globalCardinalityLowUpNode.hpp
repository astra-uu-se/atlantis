#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityLowUpNode : public ViolationInvariantNode {
 private:
  std::vector<VarNodeId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _low;
  std::vector<Int> _up;
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit GlobalCardinalityLowUpNode(std::vector<VarNodeId>&& x,
                                      std::vector<Int>&& cover,
                                      std::vector<Int>&& low,
                                      std::vector<Int>&& up, VarNodeId r);

  explicit GlobalCardinalityLowUpNode(std::vector<VarNodeId>&& x,
                                      std::vector<Int>&& cover,
                                      std::vector<Int>&& low,
                                      std::vector<Int>&& up,
                                      bool shouldHold = true);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
