#pragma once

#include "invariantgraph/invariantNode.hpp"
#include "invariants/mod.hpp"

namespace invariantgraph {

class IntModNode : public InvariantNode {
 public:
  IntModNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : InvariantNode({output}, {a, b}) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntModNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_mod", 3}};
  }

  static std::unique_ptr<IntModNode> fromModelConstraint(
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