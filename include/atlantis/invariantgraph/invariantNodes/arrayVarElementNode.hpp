#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/elementVar.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElementNode : public InvariantNode {
 private:
  Int _offset;

 public:
  ArrayVarElementNode(VarNodeId idx, std::vector<VarNodeId>&& varVector,
                      VarNodeId output, Int offset);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().front();
  }
};

}  // namespace atlantis::invariantgraph