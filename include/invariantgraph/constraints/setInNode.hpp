#pragma once
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class SetInNode : public SoftConstraintNode {
 private:
  std::vector<Int> _values;

 public:
  explicit SetInNode(VariableNode* input, std::vector<Int> values,
                     VariableNode* r)
      : SoftConstraintNode({input}, r), _values(std::move(values)) {}

  static std::unique_ptr<SetInNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace invariantgraph