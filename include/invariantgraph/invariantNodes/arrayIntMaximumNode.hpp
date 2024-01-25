#pragma once

#include <algorithm>

#include <utility>


#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/maxSparse.hpp"

namespace atlantis::invariantgraph {
class ArrayIntMaximumNode : public InvariantNode {
 public:
  ArrayIntMaximumNode(std::vector<VarNodeId>&& vars, VarNodeId output);

  ~ArrayIntMaximumNode() override = default;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace atlantis::invariantgraph
