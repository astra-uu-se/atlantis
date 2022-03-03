#pragma once

#include <utility>

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class MaxNode final : public InvariantNode {
 private:
  const std::vector<VariableNode*> _variables;

 public:
  static std::unique_ptr<MaxNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  MaxNode(std::vector<VariableNode*> variables, VariableNode* output)
      : InvariantNode(output), _variables(std::move(variables)) {}

  ~MaxNode() final = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const final;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }
};
}  // namespace invariantgraph
