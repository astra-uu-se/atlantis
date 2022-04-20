#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntNeNode : public SoftConstraintNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

 public:
  IntNeNode(VariableNode* a, VariableNode* b)
      : SoftConstraintNode({a, b}), _a(a), _b(b) {}

  static std::unique_ptr<IntNeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph