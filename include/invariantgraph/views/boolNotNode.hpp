#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {

class BoolNotNode : public VariableDefiningNode {
 private:
  VariableNode* _input;

 public:
  static std::unique_ptr<BoolNotNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BoolNotNode(VariableNode* input, VariableNode* output)
      : VariableDefiningNode({output}, {input}), _input(input) {}

  ~BoolNotNode() override = default;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* input() const noexcept { return _input; }
};

}  // namespace invariantgraph