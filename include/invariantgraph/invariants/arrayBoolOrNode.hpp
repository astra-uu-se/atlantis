#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayBoolOrNode : public VariableDefiningNode {
 private:
  VarId _violationVarId{NULL_ID};
  VarId _constZeroVarId{NULL_ID};
  VarId _sumVarId{NULL_ID};

  VariableNode _violation{SetDomain({0})};
  bool _rIsConstant;
  bool _rValue;

 public:
  static std::unique_ptr<ArrayBoolOrNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolOrNode(std::vector<VariableNode*> as, VariableNode* output)
      : VariableDefiningNode({output}, std::move(as)),
        _rIsConstant(false),
        _rValue{false} {}

  ArrayBoolOrNode(std::vector<VariableNode*> as, bool output)
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