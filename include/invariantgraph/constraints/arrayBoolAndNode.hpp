#pragma once

#include "fznparser/model.hpp"
#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public SoftConstraintNode {
 private:
  VarId _sumVarId{NULL_ID};
  const bool _rIsConstant;
  const bool _rValue;

 public:
  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolAndNode(std::vector<VariableNode*> as, VariableNode* output)
      : SoftConstraintNode(std::move(as), output),
        _rIsConstant(false),
        _rValue{false} {}

  ArrayBoolAndNode(std::vector<VariableNode*> as, bool output)
      : SoftConstraintNode(std::move(as)),
        _rIsConstant(true),
        _rValue{output} {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph