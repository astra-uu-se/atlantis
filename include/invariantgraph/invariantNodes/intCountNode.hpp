#pragma once
#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/intOffsetView.hpp"
#include "propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {
class IntCountNode : public InvariantNode {
 public:
  IntCountNode(std::vector<VarNodeId>&& vars, VarNodeId needle,
               VarNodeId count);

  ~IntCountNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] std::vector<VarNodeId> haystack() const;

  [[nodiscard]] VarNodeId needle() const;
};
}  // namespace atlantis::invariantgraph
