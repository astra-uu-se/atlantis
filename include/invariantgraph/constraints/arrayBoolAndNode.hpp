#pragma once

#include "fznparser/model.hpp"
#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolAndNode(std::vector<VariableNode*> as, VariableNode* output)
      : SoftConstraintNode(std::move(as), output) {}

  ArrayBoolAndNode(std::vector<VariableNode*> as, bool shouldHold)
      : SoftConstraintNode(std::move(as), shouldHold) {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph