#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntLinLeNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<VariableNode*> _variables;
  Int _bound;

 public:
  IntLinLeNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int bound)
      : SoftConstraintNode(
            [&] {
              return std::max<Int>(0, std::numeric_limits<Int>::max() - bound);
            },
            variables),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _bound(bound) {}

  static std::unique_ptr<IntLinLeNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int bound() const { return _bound; }

 private:
  [[nodiscard]] std::pair<Int, Int> getDomainBounds() const;
};

}  // namespace invariantgraph