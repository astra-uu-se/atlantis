#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class LeqNode final : public SoftConstraintNode {
 private:
  const std::vector<Int> _coeffs;
  const std::vector<VariableNode*> _variables;
  const Int _bound;

 public:
  LeqNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
          Int bound)
      : _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _bound(bound) {}

  static std::unique_ptr<LeqNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const final;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }

 private:
  std::pair<Int, Int> getDomainBounds() const;
};

}  // namespace invariantgraph