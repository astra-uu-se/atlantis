#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
 public:
  IntDivNode(VarNodeId numerator, VarNodeId denominator, VarNodeId quotient);

  ~IntDivNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId numerator() const noexcept;
  [[nodiscard]] VarNodeId denominator() const noexcept;
};

}  // namespace atlantis::invariantgraph