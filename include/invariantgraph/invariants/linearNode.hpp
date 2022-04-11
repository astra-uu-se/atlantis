#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {
class LinearNode : public VariableDefiningNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<VariableNode*> _variables;
  Int _offset;

 public:
  static std::unique_ptr<LinearNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  LinearNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
             VariableNode* output, Int offset = 0)
      : VariableDefiningNode({output}, variables),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _offset(offset) {}

  ~LinearNode() override = default;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

 private:
  [[nodiscard]] std::pair<Int, Int> getIntermediateDomain() const;
};
}  // namespace invariantgraph
