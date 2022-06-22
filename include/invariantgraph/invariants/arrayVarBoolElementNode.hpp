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
#ifndef NDEBUG
    assert(staticInputs().front()->isIntVar());
    for (auto* const dynamicInput : dynamicInputs()) {
      assert(!dynamicInput->isIntVar());
    }
    assert(definedVariables().front()->isIntVar());
#endif
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_var_bool_element", 3}, {"array_var_bool_element_offset", 4}};
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