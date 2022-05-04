#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {
class MinNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _variables;

 public:
  static std::unique_ptr<MinNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  MinNode(std::vector<VariableNode*> variables, VariableNode* output)
      : VariableDefiningNode({output}, variables),
        _variables(std::move(variables)) {}

  ~MinNode() override = default;

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }
};
}  // namespace invariantgraph
