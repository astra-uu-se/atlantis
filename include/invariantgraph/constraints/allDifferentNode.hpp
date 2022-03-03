#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 private:
  std::vector<VariableNode*> _variables;

 public:
  explicit AllDifferentNode(std::vector<VariableNode*> variables)
      : SoftConstraintNode(variables), _variables(std::move(variables)) {}

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() {
    return _variables;
  }
};
}  // namespace invariantgraph