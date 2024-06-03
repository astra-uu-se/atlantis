#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

class IntModNode : public InvariantNode {
 public:
  IntModNode(VarNodeId numerator, VarNodeId denominator, VarNodeId remainder);

  ~IntModNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  void propagate(InvariantGraph& graph) override;

  [[nodiscard]] VarNodeId numerator() const;
  [[nodiscard]] VarNodeId denominator() const;
  [[nodiscard]] VarNodeId remainder() const;
};

}  // namespace atlantis::invariantgraph
