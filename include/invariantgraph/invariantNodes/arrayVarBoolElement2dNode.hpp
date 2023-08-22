#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/invariantNode.hpp"
#include "invariants/element2dVar.hpp"

namespace invariantgraph {

class ArrayVarBoolElement2dNode : public InvariantNode {
 private:
  const size_t _numRows;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayVarBoolElement2dNode(VarNodeId idx1, VarNodeId idx2,
                            std::vector<VarNodeId>&& x, VarNodeId output,
                            size_t numRows, Int offset1, Int offset2)
      : InvariantNode({output}, {idx1, idx2}, std::move(x)),
        _numRows(numRows),
        _offset1(offset1),
        _offset2(offset2) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_bool_element2d_nonshifted_flat", 7}};
  }

  static std::unique_ptr<ArrayVarBoolElement2dNode> fromModelConstraint(
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