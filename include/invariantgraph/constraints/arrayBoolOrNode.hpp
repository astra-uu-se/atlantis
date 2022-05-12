#pragma once

#include <utility>

#include "fznparser/model.hpp"
#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class ArrayBoolOrNode : public SoftConstraintNode {
 private:
  VarId _sumVarId{NULL_ID};
  const bool _rIsConstant;
  const bool _rValue;

 public:
  static std::unique_ptr<ArrayBoolOrNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolOrNode(std::vector<VariableNode*> as, VariableNode* output)
      : SoftConstraintNode(std::move(as), output),
        _rIsConstant(false),
        _rValue{false} {}

  ArrayBoolOrNode(std::vector<VariableNode*> as, bool output)
      : SoftConstraintNode(std::move(as)),
        _rIsConstant(true),
        _rValue{output} {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph