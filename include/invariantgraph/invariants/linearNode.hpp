#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {
class LinearNode : public VariableDefiningNode {
 private:
  std::vector<Int> _coeffs;
  Int _offset;
  VarId _intermediateVarId{NULL_ID};

 public:
  static std::unique_ptr<LinearNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  LinearNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
             VariableNode* output, Int offset = 0)
      : VariableDefiningNode({output}, variables),
        _coeffs(std::move(coeffs)),
        _offset(offset) {
    assert(definedVariables().size() == 1);
    assert(definedVariables().front() == output);
    assert(staticInputs().size() == variables.size());
#ifndef NDEBUG
    for (size_t i = 0; i < variables.size(); ++i) {
      assert(variables[i] = staticInputs()[i]);
    }
#endif
    assert(dynamicInputs().empty());
  }

  ~LinearNode() override = default;

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }
};
}  // namespace invariantgraph
