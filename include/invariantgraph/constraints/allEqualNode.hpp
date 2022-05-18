#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {
class AllEqualNode : public SoftConstraintNode {
 private:
  VarId _allDifferentViolationVarId{NULL_ID};

 public:
  explicit AllEqualNode(std::vector<VariableNode*> variables, VariableNode* r)
      : SoftConstraintNode(variables, r) {}

  explicit AllEqualNode(std::vector<VariableNode*> variables,
                        bool shouldHold = true)
      : SoftConstraintNode(variables, shouldHold) {}

  static std::unique_ptr<AllEqualNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph