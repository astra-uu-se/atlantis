#pragma once

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntLinEqNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<VariableNode*> _variables;
  Int _c;

 public:
  static std::unique_ptr<IntLinEqNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  IntLinEqNode(std::vector<Int> coeffs,
                             std::vector<VariableNode*> variables, Int c)
      : _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _c(c) {}

  VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }

 private:
  std::pair<Int, Int> getDomainBounds() const;
};

}  // namespace invariantgraph