#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {
class MaxNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<MaxNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  MaxNode(std::vector<VariableNode*> variables, VariableNode* output)
      : VariableDefiningNode({output}, variables) {}

  ~MaxNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph
