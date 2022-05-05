#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {

class Bool2IntNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<Bool2IntNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  Bool2IntNode(VariableNode* staticInput, VariableNode* output)
      : VariableDefiningNode({output}, {staticInput}) {}

  ~Bool2IntNode() override = default;

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* input() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph