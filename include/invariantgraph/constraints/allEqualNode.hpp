#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"
#include "views/equalView.hpp"

namespace invariantgraph {
class AllEqualNode : public SoftConstraintNode {
 private:
  VarId _allDifferentViolationVarId{NULL_ID};

 public:
  explicit AllEqualNode(std::vector<VariableNode*> variables,
                        VariableNode* r = nullptr)
      : SoftConstraintNode(variables, r) {}

  static std::unique_ptr<AllEqualNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph