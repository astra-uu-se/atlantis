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
  ArrayVarElement2dNode(InvariantGraph& graph,

                        VarNodeId idx1, VarNodeId idx2,
                        std::vector<VarNodeId>&& flatVarMatrix,
                        VarNodeId output, size_t numRows, Int offset1,
                        Int offset2);

  ArrayVarElement2dNode(InvariantGraph& graph,

                        VarNodeId idx1, VarNodeId idx2,
                        std::vector<std::vector<VarNodeId>>&& varMatrix,
                        VarNodeId output, Int offset1, Int offset2);

  void init(InvariantNodeId) override;

  void updateState() override;

  [[nodiscard]] bool canBeReplaced() const override;

  [[nodiscard]] bool replace() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] VarNodeId at(Int row, Int col);

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }

  size_t numCols() const noexcept {
    return dynamicInputVarNodeIds().size() / _numRows;
  }
};

}  // namespace atlantis::invariantgraph
