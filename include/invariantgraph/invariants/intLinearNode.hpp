#pragma once

#include <utility>

#include "invariantgraph/invariantNode.hpp"

namespace invariantgraph {
class IntLinearNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _definingCoefficient;
  Int _sum;
  VarId _intermediateVarId{NULL_ID};

 public:
  IntLinearNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                VariableNode* output, Int definingCoefficient, Int sum)
      : InvariantNode({output}, std::move(variables)),
        _coeffs(std::move(coeffs)),
        _definingCoefficient(definingCoefficient),
        _sum(sum) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~IntLinearNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_lin_eq", 3}};
  }

  static std::unique_ptr<IntLinearNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }
};
}  // namespace invariantgraph
