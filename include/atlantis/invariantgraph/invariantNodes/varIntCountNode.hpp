#pragma once
#include <algorithm>
#include <utility>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {
class VarIntCountNode : public InvariantNode {
 public:
  VarIntCountNode(std::vector<VarNodeId>&& vars, VarNodeId needle,
                  VarNodeId count);

  ~VarIntCountNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] std::vector<VarNodeId> haystack() const;

  [[nodiscard]] VarNodeId needle() const;
};
}  // namespace atlantis::invariantgraph
