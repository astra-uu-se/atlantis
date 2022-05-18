#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolLessEqual.hpp"
#include "constraints/boolLessThan.hpp"
#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class BoolLeNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  BoolLeNode(VariableNode* a, VariableNode* b, VariableNode* r)
      : SoftConstraintNode({a, b}, r) {}
  BoolLeNode(VariableNode* a, VariableNode* b, bool shouldHold)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::unique_ptr<BoolLeNode> fromModelConstraint(
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