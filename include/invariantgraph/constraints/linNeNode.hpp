#pragma once

#include <fznparser/model.hpp>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class LinNeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  Int _c;
  VarId _sumVarId{NULL_ID};

 public:
  static std::unique_ptr<LinNeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  LinNeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
            Int c, VariableNode* r = nullptr)
      : SoftConstraintNode(variables, r), _coeffs(std::move(coeffs)), _c(c) {
    assert(staticInputs().size() == variables.size());
#ifndef NDEBUG
    for (size_t i = 0; i < variables.size(); ++i) {
      assert(variables[i] = staticInputs()[i]);
    }
#endif
    assert(r == nullptr || violation() == r);
    assert(dynamicInputs().empty());
  }

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }
};

}  // namespace invariantgraph