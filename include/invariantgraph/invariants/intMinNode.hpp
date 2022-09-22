#pragma once

#include <cmath>

#include "invariantgraph/invariantNode.hpp"
#include "invariants/binaryMin.hpp"

namespace invariantgraph {

class IntMinNode : public InvariantNode {
 public:
  IntMinNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : InvariantNode({output}, {a, b}) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntMinNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_min", 3}};
  }

  static std::unique_ptr<IntMinNode> fromModelConstraint(
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