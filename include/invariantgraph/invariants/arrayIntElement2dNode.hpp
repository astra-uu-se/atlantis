#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/element2dConst.hpp"

namespace invariantgraph {

class ArrayIntElement2dNode : public VariableDefiningNode {
 private:
  const std::vector<std::vector<Int>> _parMatrix;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayIntElement2dNode(VariableNode* idx1, VariableNode* idx2,
                        std::vector<std::vector<Int>> parMatrix,
                        VariableNode* output, Int offset1, Int offset2)
      : VariableDefiningNode({output}, {idx1, idx2}),
        _parMatrix(std::move(parMatrix)),
        _offset1(offset1),
        _offset2(offset2) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_int_element2d_nonshifted_flat", 7}};
  }

  static std::unique_ptr<ArrayIntElement2dNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* idx1() const noexcept {
    return staticInputs().front();
  }

  [[nodiscard]] VariableNode* idx2() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph