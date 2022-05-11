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
      : VariableDefiningNode({output}, as) {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph