#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/element2dVar.hpp"

namespace invariantgraph {

class ArrayVarBoolElement2dNode : public VariableDefiningNode {
 private:
  const size_t _numRows;
  const Int _offset1;
  const Int _offset2;

 public:
  ArrayVarBoolElement2dNode(VariableNode* idx1, VariableNode* idx2,
                            const std::vector<VariableNode*>& x,
                            VariableNode* output, size_t numRows, Int offset1,
                            Int offset2)
      : VariableDefiningNode({output}, {idx1, idx2}, x),
        _numRows(numRows),
        _offset1(offset1),
        _offset2(offset2) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
    assert(std::all_of(
        dynamicInputs().begin(), dynamicInputs().end(),
        [&](auto* const dynamicInput) { return !dynamicInput->isIntVar(); }));
    assert(std::all_of(definedVariables().begin(), definedVariables().end(),
                       [&](auto* const definedVariable) {
                         return definedVariable->isIntVar();
                       }));
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_bool_element2d_nonshifted_flat", 7}};
  }

  static std::unique_ptr<ArrayVarBoolElement2dNode> fromModelConstraint(
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