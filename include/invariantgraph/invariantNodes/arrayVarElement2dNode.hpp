#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/element2dVar.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElement2dNode : public InvariantNode {
 private:
  const size_t _numRows;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayVarElement2dNode(VarNodeId idx1, VarNodeId idx2,
                        std::vector<VarNodeId>&& flatVarMatrix,
                        VarNodeId output, size_t numRows, Int offset1,
                        Int offset2);

  ArrayVarElement2dNode(VarNodeId idx1, VarNodeId idx2,
                        std::vector<std::vector<VarNodeId>>&& varMatrix,
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