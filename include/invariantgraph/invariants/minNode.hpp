#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {
class MinNode : public InvariantNode {
 private:
  VariableNode* _output;

 public:
  static std::unique_ptr<MinNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  MinNode(std::vector<VariableNode*> variables, VariableNode* output)
      : InvariantNode({output}, std::move(variables)), _output{output} {}

  ~MinNode() override = default;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;
};
}  // namespace invariantgraph
