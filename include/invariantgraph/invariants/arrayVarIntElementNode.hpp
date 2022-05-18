#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayVarIntElementNode(VariableNode* b, std::vector<VariableNode*> as,
                         VariableNode* output)
      : VariableDefiningNode({output}, {b}, {as}) {
#ifndef NDEBUG
    assert(staticInputs().front()->isIntVar());
    for (auto* const dynamicInput : dynamicInputs()) {
      assert(dynamicInput->isIntVar());
    }
#endif
  }

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph