#pragma once

#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {
class BoolLinearNode : public VariableDefiningNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset;
  VarId _intermediateVarId{NULL_ID};

 public:
  static std::unique_ptr<BoolLinearNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BoolLinearNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
                 VariableNode* output, Int offset = 0)
      : VariableDefiningNode({output}, variables),
        _coeffs(std::move(coeffs)),
        _offset(offset) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(!staticInput->isIntVar());
    }
#endif
  }

  ~BoolLinearNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }
};
}  // namespace invariantgraph
