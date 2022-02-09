#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 private:
  std::vector<std::shared_ptr<VariableNode>> _variables;

 public:
  explicit AllDifferentNode(
      std::vector<std::shared_ptr<VariableNode>> variables)
      : _variables(std::move(variables)) {}

  static std::shared_ptr<AllDifferentNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<std::shared_ptr<VariableNode>(
          std::shared_ptr<fznparser::Variable>)>& variableMap);

  VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const override;

  [[nodiscard]] const std::vector<std::shared_ptr<VariableNode>>& variables() {
    return _variables;
  }
};
}  // namespace invariantgraph