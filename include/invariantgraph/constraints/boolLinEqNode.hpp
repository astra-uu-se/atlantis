#pragma once

#include <fznparser/model.hpp>

#include "constraints/equal.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {

class BoolLinEqNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _cVarId{NULL_ID};
  VarId _sumVarId{NULL_ID};

 public:
  static std::unique_ptr<BoolLinEqNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BoolLinEqNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                Int c, VariableNode* r)
      : SoftConstraintNode(variables, r), _coeffs(std::move(coeffs)), _c(c) {}
  BoolLinEqNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                Int c, bool shouldHold)
      : SoftConstraintNode(variables, shouldHold),
        _coeffs(std::move(coeffs)),
        _c(c) {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& node) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph