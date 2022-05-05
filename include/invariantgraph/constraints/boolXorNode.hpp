#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class BoolXorNode : public SoftConstraintNode {
 public:
  BoolXorNode(VariableNode* a, VariableNode* b, VariableNode* r = nullptr)
      : SoftConstraintNode({a, b}, r) {
    assert(staticInputs().size() == 2);
    assert(staticInputs().front() == a);
    assert(staticInputs().back() == b);
    assert(r == nullptr || violation() == r);
    assert(dynamicInputs().empty());
  }

  static std::unique_ptr<BoolXorNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept {
    return staticInputs().front();
  }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph