#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "propagation/invariants/element2dVar.hpp"

namespace atlantis::invariantgraph {

class ArrayVarBoolElement2dNode : public InvariantNode {
 private:
  const size_t _numRows;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayVarBoolElement2dNode(VarNodeId idx1, VarNodeId idx2,
                            std::vector<VarNodeId>&& x, VarNodeId output,
                            size_t numRows, Int offset1, Int offset2);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"array_var_bool_element2d_nonshifted_flat", 7}};
  }

  static std::unique_ptr<ArrayVarBoolElement2dNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVars(InvariantGraph&, propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace invariantgraph