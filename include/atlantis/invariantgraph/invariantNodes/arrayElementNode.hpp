#pragma once

#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/types.hpp"

namespace atlantis::invariantgraph {

class ArrayElementNode : public InvariantNode {
 private:
  std::vector<Int> _parVector;
  Int _offset;

 public:
  ArrayElementNode(std::vector<Int>&& parVector, VarNodeId idx,
                   VarNodeId output, Int offset);

  ArrayElementNode(std::vector<bool>&& parVector, VarNodeId idx,
                   VarNodeId output, Int offset);

  void updateState(InvariantGraph& graph) override;

  void registerOutputVars(InvariantGraph&,
                          propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept {
    return _parVector;
  }
  [[nodiscard]] VarNodeId idx() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace atlantis::invariantgraph
