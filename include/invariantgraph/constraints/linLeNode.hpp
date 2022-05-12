#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class LinLeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  VarId _sumVarId{NULL_ID};

 public:
  LinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
            Int bound, VariableNode* r = nullptr)
      : SoftConstraintNode(variables, r),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}

  static std::unique_ptr<LinLeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }
};

}  // namespace invariantgraph