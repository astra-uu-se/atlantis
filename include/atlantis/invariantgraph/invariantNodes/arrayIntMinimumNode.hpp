#pragma once

#include <algorithm>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/minSparse.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
 public:
  ArrayIntMinimumNode(std::vector<VarNodeId>&& vars, VarNodeId output);

  ~ArrayIntMinimumNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
