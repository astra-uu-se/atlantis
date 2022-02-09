#pragma once

#include <utility>

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class LinearInvariantNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<std::shared_ptr<VariableNode>> _variables;

 public:
  static std::shared_ptr<LinearInvariantNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<std::shared_ptr<VariableNode>(
          std::shared_ptr<fznparser::Variable>)>& variableMap);

  LinearInvariantNode(std::vector<Int> coeffs,
                      std::vector<std::shared_ptr<VariableNode>> variables,
                      std::shared_ptr<VariableNode> output)
      : InvariantNode(std::move(output)),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)) {}

  ~LinearInvariantNode() override = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const override;
};
}  // namespace invariantgraph
