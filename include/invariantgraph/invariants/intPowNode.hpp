#pragma once

#include <cmath>

#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/pow.hpp"

namespace invariantgraph {

class IntPowNode : public VariableDefiningNode {
 public:
  IntPowNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : VariableDefiningNode({output}, {a, b}) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntPowNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"int_pow", 3}, {"int_pow_fixed", 3}};
  }

  static std::unique_ptr<IntPowNode> fromModelConstraint(
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