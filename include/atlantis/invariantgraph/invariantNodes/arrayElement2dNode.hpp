#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

class ArrayElement2dNode : public InvariantNode {
 private:
  std::vector<std::vector<Int>> _parMatrix;
  Int _offset1;
  Int _offset2;
  bool _isIntMatrix;

 public:
  ArrayElement2dNode(VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<Int>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  ArrayElement2dNode(VarNodeId idx1, VarNodeId idx2,
                     std::vector<std::vector<bool>>&& parMatrix,
                     VarNodeId output, Int offset1, Int offset2);

  void init(InvariantGraph&, const InvariantNodeId&) override;

  void updateState(InvariantGraph&) override;

  [[nodiscard]] bool canBeReplaced(const InvariantGraph&) const override;

  [[nodiscard]] bool replace(InvariantGraph&) override;

  void registerOutputVars(InvariantGraph&, propagation::SolverBase&) override;

  void registerNode(InvariantGraph&, propagation::SolverBase&) override;

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
