#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntEqNode : public SoftConstraintNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

  inline static auto violationUb = [](auto a, auto b) {
    const auto& [al, au] = a->bounds();
    const auto& [bl, bu] = b->bounds();

    auto diff1 = std::abs(al - bu);
    auto diff2 = std::abs(au - bl);

    return std::max(diff1, diff2);
  };

 public:
  IntEqNode(VariableNode* a, VariableNode* b)
      : SoftConstraintNode([&] { return violationUb(a, b); }, {a, b}),
        _a(a),
        _b(b) {}

  static std::unique_ptr<IntEqNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph