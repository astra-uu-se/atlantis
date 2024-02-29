#pragma once

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"

namespace atlantis::invariantgraph {

class ArrayElement2dNode : public InvariantNode {
 private:
  std::vector<std::vector<Int>> _parMatrix;
  Int _offset1;
  Int _offset2;

 public:
  ArrayElement2dNode(VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<Int>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  ArrayElement2dNode(VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<bool>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
