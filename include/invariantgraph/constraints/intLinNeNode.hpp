#pragma once

#include <fznparser/model.hpp>

#include "constraints/notEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/linear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class IntLinNeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _sumVarId{NULL_ID};

 public:
  IntLinNeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int c, VariableNode* r)
      : SoftConstraintNode(variables, r), _coeffs(std::move(coeffs)), _c(c) {}
  IntLinNeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int c, bool shouldHold)
      : SoftConstraintNode(variables, shouldHold),
        _coeffs(std::move(coeffs)),
        _c(c) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"int_lin_ne", 3}, {"int_lin_ne_reif", 4}};
  }

  static std::unique_ptr<IntLinNeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph