#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/linear.hpp"
#include "views/greaterThanView.hpp"
#include "views/lessEqualView.hpp"

namespace invariantgraph {

class IntLinLeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  VarId _sumVarId{NULL_ID};

 public:
  IntLinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int bound, VariableNode* r)
      : SoftConstraintNode(variables, r),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}
  IntLinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int bound, bool shouldHold)
      : SoftConstraintNode(variables, shouldHold),
        _coeffs(std::move(coeffs)),
        _bound(bound) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"int_lin_le", 3}, {"int_lin_le_reif", 4}};
  }

  static std::unique_ptr<IntLinLeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }
};

}  // namespace invariantgraph