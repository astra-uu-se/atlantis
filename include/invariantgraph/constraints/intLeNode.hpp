#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "constraints/lessThan.hpp"
#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class IntLeNode : public SoftConstraintNode {
 public:
  IntLeNode(VariableNode* a, VariableNode* b, VariableNode* r)
      : SoftConstraintNode({a, b}, r) {}
  IntLeNode(VariableNode* a, VariableNode* b, bool shouldHold)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_le", 2},
                                                            {"int_le_reif", 3}};
  }

  static std::unique_ptr<IntLeNode> fromModelConstraint(
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