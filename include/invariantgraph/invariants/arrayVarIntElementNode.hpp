#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayVarIntElementNode(VariableNode* b, std::vector<VariableNode*> as,
                         VariableNode* output)
      : VariableDefiningNode({output}, {b}, {as}) {
    assert(definedVariables().size() == 1);
    assert(definedVariables().front() == output);
    assert(staticInputs().size() == 1);
    assert(staticInputs()[0] == b);
    assert(dynamicInputs().size() == as.size());
#ifndef NDEBUG
    for (size_t i = 0; i < as.size(); ++i) {
      assert(as[i] = dynamicInputs()[i]);
    }
#endif
  }

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph