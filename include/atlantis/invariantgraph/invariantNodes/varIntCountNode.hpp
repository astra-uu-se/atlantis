#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class VarIntCountNode : public InvariantNode {
 public:
  VarIntCountNode(std::vector<VarNodeId>&& vars, VarNodeId needle,
                  VarNodeId count);

  ~VarIntCountNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  void propagate(InvariantGraph& graph) override;

  [[nodiscard]] std::vector<VarNodeId> haystack() const;

  [[nodiscard]] VarNodeId needle() const;
};
}  // namespace atlantis::invariantgraph
