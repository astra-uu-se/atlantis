#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

class ArrayVarElement2dNode : public InvariantNode {
 private:
  size_t _numRows;
  Int _offset1;
  Int _offset2;

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

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph& graph) override;

  [[nodiscard]] VarNodeId at(Int row, Int col);

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
