#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class BoolNotNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<BoolNotNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BoolNotNode(VariableNode* staticInput, VariableNode* output)
      : VariableDefiningNode({output}, {staticInput}) {}

  ~BoolNotNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* input() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph