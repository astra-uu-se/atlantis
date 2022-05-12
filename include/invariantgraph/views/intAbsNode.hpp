#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class IntAbsNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<IntAbsNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  IntAbsNode(VariableNode* staticInput, VariableNode* output)
      : VariableDefiningNode({output}, {staticInput}) {}

  ~IntAbsNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* input() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph