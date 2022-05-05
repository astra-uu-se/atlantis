#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public VariableDefiningNode {
 private:
  VarId _sumVarId{NULL_ID};

 public:
  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolAndNode(std::vector<VariableNode*> as, VariableNode* output)
      : VariableDefiningNode({output}, as) {
    assert(definedVariables().size() == 1);
    assert(definedVariables().front() == output);
    assert(staticInputs().size() == as.size());
#ifndef NDEBUG
    for (size_t i = 0; i < as.size(); ++i) {
      assert(as[i] = staticInputs()[i]);
    }
#endif
    assert(dynamicInputs().empty());
  }

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;
};

}  // namespace invariantgraph