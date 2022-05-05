#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class LtNode : public SoftConstraintNode {
 public:
  LtNode(VariableNode* a, VariableNode* b, VariableNode* r)
      : SoftConstraintNode({a, b}, r) {}

  static std::unique_ptr<LtNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept {
    return staticInputs().front();
  }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph