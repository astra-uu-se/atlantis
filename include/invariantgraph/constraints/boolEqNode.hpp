#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolXor.hpp"

namespace invariantgraph {

class BoolEqNode : public SoftConstraintNode {
 public:
  BoolEqNode(VariableNode* a, VariableNode* b, VariableNode* r = nullptr)
      : SoftConstraintNode({a, b}, r) {}
  BoolEqNode(VariableNode* a, VariableNode* b, bool shouldHold)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::unique_ptr<BoolEqNode> fromModelConstraint(
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