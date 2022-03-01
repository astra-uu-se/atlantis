#pragma once

#include <utility>

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class MinNode : public InvariantNode {
 private:
  std::vector<VariableNode*> _variables;

 public:
  static std::unique_ptr<MinNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  MinNode(std::vector<VariableNode*> variables, VariableNode* output)
      : InvariantNode(output), _variables(std::move(variables)) {}

  ~MinNode() override = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }
};
}  // namespace invariantgraph
