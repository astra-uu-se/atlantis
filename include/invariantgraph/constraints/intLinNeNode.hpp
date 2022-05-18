#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class IntLinNeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _sumVarId{NULL_ID};
  VarId _intermediate{NULL_ID};

 public:
  static std::unique_ptr<IntLinNeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  IntLinNeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int c, VariableNode* r)
      : SoftConstraintNode(variables, r), _coeffs(std::move(coeffs)), _c(c) {}
  IntLinNeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int c, bool shouldHold)
      : SoftConstraintNode(variables, shouldHold),
        _coeffs(std::move(coeffs)),
        _c(c) {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph