#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntDivNode : public InvariantNode {
 public:
  IntDivNode(VarNodeId numerator, VarNodeId denominator, VarNodeId quotient);

  ~IntDivNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  void propagate(InvariantGraph& graph) override;

  [[nodiscard]] bool replace(InvariantGraph& graph) override;

  [[nodiscard]] VarNodeId numerator() const noexcept;
  [[nodiscard]] VarNodeId denominator() const noexcept;
  [[nodiscard]] VarNodeId quotient() const noexcept;
};

}  // namespace atlantis::invariantgraph
