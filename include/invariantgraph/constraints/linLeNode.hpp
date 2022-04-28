#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class LinLeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<VariableNode*> _variables;
  Int _bound;
  VarId _boundVarId{NULL_ID};
  VarId _sumVarId{NULL_ID};

 public:
  LinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
            Int bound)
      : SoftConstraintNode(variables),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _bound(bound) {}

  static std::unique_ptr<LinLeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }
};

}  // namespace invariantgraph