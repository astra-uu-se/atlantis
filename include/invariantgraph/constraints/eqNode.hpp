#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class EqNode : public SoftConstraintNode {
 public:
  EqNode(VariableNode* a, VariableNode* b, VariableNode* r = nullptr)
      : SoftConstraintNode({a, b}, r) {}

  static std::unique_ptr<EqNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* a() const noexcept {
    return staticInputs().front();
  }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph