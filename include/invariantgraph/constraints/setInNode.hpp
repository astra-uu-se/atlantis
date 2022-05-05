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
      : SoftConstraintNode({input}, r), _values(std::move(values)) {
    assert(staticInputs().size() == 1);
    assert(staticInputs().front() == input);
    assert(r == nullptr || violation() == r);
    assert(dynamicInputs().empty());
  }

  static std::unique_ptr<SetInNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace invariantgraph