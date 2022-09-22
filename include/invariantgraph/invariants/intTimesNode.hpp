#pragma once

#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/times.hpp"

namespace invariantgraph {

class IntTimesNode : public VariableDefiningNode {
 public:
  IntTimesNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : VariableDefiningNode({output}, {a, b}) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
  }

  ~IntTimesNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_times", 3}};
  }

  static std::unique_ptr<IntTimesNode> fromModelConstraint(
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