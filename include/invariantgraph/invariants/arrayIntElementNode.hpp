#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayIntElementNode : public VariableDefiningNode {
 private:
  std::vector<Int> _as;
  VariableNode* _b;

 public:
  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayIntElementNode(std::vector<Int> as, VariableNode* b,
                      VariableNode* output)
      : VariableDefiningNode({output}, false, {b}), _as(std::move(as)), _b(b) {}

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept { return _as; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph