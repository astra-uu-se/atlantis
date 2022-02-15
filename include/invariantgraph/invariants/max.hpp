#pragma once

#include <utility>

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class MaxInvariantNode : public InvariantNode {
 private:
  std::vector<std::shared_ptr<VariableNode>> _variables;

 public:
  static std::shared_ptr<MaxInvariantNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<std::shared_ptr<VariableNode>(
          std::shared_ptr<fznparser::Variable>)>& variableMap);

  MaxInvariantNode(std::vector<std::shared_ptr<VariableNode>> variables,
                   std::shared_ptr<VariableNode> output)
      : InvariantNode(std::move(output)), _variables(std::move(variables)) {}

  ~MaxInvariantNode() override = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const override;

  [[nodiscard]] const std::vector<std::shared_ptr<VariableNode>>& variables()
      const {
    return _variables;
  }
};
}  // namespace invariantgraph
