#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayVarBoolElementNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _as;
  VariableNode* _b;

 public:
  static std::unique_ptr<ArrayVarBoolElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayVarBoolElementNode(std::vector<VariableNode*> as, VariableNode* b,
                          VariableNode* output)
      : VariableDefiningNode({output}, {as}), _as(std::move(as)), _b(b) {
    // No way to add this as an input in addition to the as vector. So we do it
    // here explicitly.
    markAsInput(b);
  }

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() const noexcept {
    return _as;
  }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph