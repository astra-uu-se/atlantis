#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayIntElementNode : public InvariantNode {
 private:
  std::vector<Int> _as;
  VariableNode* _output;

 public:
  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayIntElementNode(std::vector<Int> as, VariableNode* b,
                      VariableNode* output)
      : InvariantNode({output}, {b}), _as(std::move(as)), _output{output} {}

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept { return _as; }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph