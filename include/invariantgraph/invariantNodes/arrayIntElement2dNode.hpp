#pragma once




#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/element2dConst.hpp"

namespace atlantis::invariantgraph {

class ArrayIntElement2dNode : public InvariantNode {
 private:
  const std::vector<std::vector<Int>> _parMatrix;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayIntElement2dNode(VarNodeId idx1, VarNodeId idx2,
                        std::vector<std::vector<Int>>&& parMatrix,
                        VarNodeId output, Int offset1, Int offset2);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_int_element2d_nonshifted_flat", 7}};
  }

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