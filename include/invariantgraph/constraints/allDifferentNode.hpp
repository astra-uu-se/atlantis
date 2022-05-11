#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 public:
  explicit AllDifferentNode(std::vector<VariableNode*> variables,
                            VariableNode* r)
      : SoftConstraintNode(variables, r) {}

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph