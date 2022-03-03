#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class AllDifferentNode final : public SoftConstraintNode {
 private:
  const std::vector<VariableNode*> _variables;

 public:
  explicit AllDifferentNode(std::vector<VariableNode*> variables)
      : _variables(std::move(variables)) {}

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const final;

  [[nodiscard]] const std::vector<VariableNode*>& variables() {
    return _variables;
  }
};
}  // namespace invariantgraph