#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/element2dConst.hpp"

namespace invariantgraph {

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

  static std::unique_ptr<ArrayIntElement2dNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  void registerOutputVariables(InvariantGraph&, Engine& engine) override;

  void registerNode(InvariantGraph&, Engine& engine) override;

  [[nodiscard]] VarNodeId idx1() const noexcept {
    return staticInputVarNodeIds().front();
  }

  [[nodiscard]] VarNodeId idx2() const noexcept {
    return staticInputVarNodeIds().back();
  }
};

}  // namespace invariantgraph