#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _as;
  VarId _sumVarId{NULL_ID};

 public:
  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolAndNode(std::vector<VariableNode*> as, VariableNode* output)
      : VariableDefiningNode({output}, false, as), _as(std::move(as)) {}

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() const noexcept {
    return _as;
  }
};

}  // namespace invariantgraph