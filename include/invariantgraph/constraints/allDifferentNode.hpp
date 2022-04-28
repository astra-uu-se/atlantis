#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 private:
  std::vector<VariableNode*> _variables;

 public:
  explicit AllDifferentNode(std::vector<VariableNode*> variables)
      : SoftConstraintNode(false, variables),
        _variables(std::move(variables)) {}

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() {
    return _variables;
  }
};
}  // namespace invariantgraph