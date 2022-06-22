#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/elementVar.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public VariableDefiningNode {
 private:
  const Int _offset;

 public:
  ArrayVarIntElementNode(VariableNode* b, std::vector<VariableNode*> as,
                         VariableNode* output, Int offset)
      : VariableDefiningNode({output}, {b}, as), _offset(offset) {
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
        {"array_var_int_element", 3}, {"array_var_int_element_offset", 4}};
  }

  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph