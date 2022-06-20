#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/element2dVar.hpp"

namespace invariantgraph {

class ArrayVarIntElement2dNode : public VariableDefiningNode {
 private:
  const size_t _numRows;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayVarIntElement2dNode(VariableNode* idx1, VariableNode* idx2,
                           std::vector<VariableNode*> as, VariableNode* output,
                           size_t numRows, Int offset1, Int offset2)
      : VariableDefiningNode({output}, {idx1, idx2}, as),
        _numRows(numRows),
        _offset1(offset1),
        _offset2(offset2) {
#ifndef NDEBUG
    assert(staticInputs().front()->isIntVar());
    for (auto* const dynamicInput : dynamicInputs()) {
      assert(dynamicInput->isIntVar());
    }
#endif
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_int_element2d_nonshifted_flat", 7}};
  }

  static std::unique_ptr<ArrayVarIntElement2dNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

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