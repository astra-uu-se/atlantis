#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolAnd.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {

class BoolAndNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  BoolAndNode(VariableNode* a, VariableNode* b, VariableNode* r)
      : SoftConstraintNode({a, b}, r) {}
  BoolAndNode(VariableNode* a, VariableNode* b, bool shouldHold)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool_and", 3}};
  }

  static std::unique_ptr<BoolAndNode> fromModelConstraint(
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