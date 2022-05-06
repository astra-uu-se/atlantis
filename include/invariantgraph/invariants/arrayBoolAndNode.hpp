#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public VariableDefiningNode {
 private:
  VarId _sumVarId{NULL_ID};

  VariableNode _violation{SetDomain({0})};
  bool _rIsConstant;
  bool _rValue;

 public:
  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolAndNode(std::vector<VariableNode*> as, VariableNode* output)
      : VariableDefiningNode({output}, std::move(as)),
        _rIsConstant(false),
        _rValue{false} {}

  ArrayBoolAndNode(std::vector<VariableNode*> as, bool output)
      : VariableDefiningNode({&_violation}, std::move(as)),
        _rIsConstant(true),
        _rValue{output} {}

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  VariableNode* violation() override;
};

}  // namespace invariantgraph