#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class LinEqNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<VariableNode*> _variables;
  Int _c;
  VarId _cVarId{NULL_ID};
  VarId _sumVarId{NULL_ID};

 public:
  static std::unique_ptr<LinEqNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  LinEqNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
            Int c)
      : SoftConstraintNode(variables),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _c(c) {}

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& node, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph