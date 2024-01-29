#pragma once

#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/invariants/exists.hpp"
#include "propagation/invariants/globalCardinalityOpen.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/equalConst.hpp"
#include "propagation/views/notEqualConst.hpp"
#include "propagation/violationInvariants/equal.hpp"
#include "propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {
class GlobalCardinalityNode : public InvariantNode {
 private:
  const std::vector<Int> _cover;
  std::vector<propagation::VarId> _intermediate{};

 public:
  explicit GlobalCardinalityNode(std::vector<VarNodeId>&& inputs,
                                 std::vector<Int>&& cover,
                                 std::vector<VarNodeId>&& counts);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph