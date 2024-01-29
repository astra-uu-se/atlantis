#pragma once
#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/countConst.hpp"
#include "propagation/views/intOffsetView.hpp"
#include "propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {
class IntCountNode : public InvariantNode {
 private:
  Int _needle;

 public:
  IntCountNode(std::vector<VarNodeId>&& vars, Int needle, VarNodeId count);

  ~IntCountNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<VarNodeId>& haystack() const;

  [[nodiscard]] Int needle() const;
};
}  // namespace atlantis::invariantgraph
