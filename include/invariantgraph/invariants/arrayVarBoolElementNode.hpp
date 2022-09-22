#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class ArrayVarBoolElementNode : public VariableDefiningNode {
 private:
  const Int _offset;

 public:
  ArrayVarBoolElementNode(VariableNode* b, std::vector<VariableNode*> as,
                          VariableNode* output, Int offset)
      : VariableDefiningNode({output}, {b}, as), _offset(offset) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
    assert(std::all_of(
        dynamicInputs().begin(), dynamicInputs().end(),
        [&](auto* const dynamicInput) { return !dynamicInput->isIntVar(); }));
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_bool_element", 3},
        {"array_var_bool_element_nonshifted", 3}};
  }

  static std::unique_ptr<ArrayVarBoolElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph