#pragma once
#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/views/intOffsetView.hpp"
#include "propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;

 public:
  IntLinearNode(std::vector<Int>&& coeffs, std::vector<VarNodeId>&& vars,
                VarNodeId output);

  ~IntLinearNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};
}  // namespace atlantis::invariantgraph
