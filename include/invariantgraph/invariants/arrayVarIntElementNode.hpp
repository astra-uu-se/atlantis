#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public InvariantNode {
 private:
  VariableNode* _output;

 public:
  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayVarIntElementNode(VariableNode* b, std::vector<VariableNode*> as,
                         VariableNode* output)
      : InvariantNode({output}, {b}, {std::move(as)}), _output(output) {}

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph