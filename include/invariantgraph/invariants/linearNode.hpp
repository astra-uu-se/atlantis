#pragma once

#include <utility>

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class LinearNode final : public InvariantNode {
 private:
  const std::vector<Int> _coeffs;
  const std::vector<VariableNode*> _variables;

 public:
  static std::unique_ptr<LinearNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  LinearNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
             VariableNode* output)
      : InvariantNode(output),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)) {}

  ~LinearNode() final = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const final;
};
}  // namespace invariantgraph
