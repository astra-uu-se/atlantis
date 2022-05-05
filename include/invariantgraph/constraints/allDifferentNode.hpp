#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 public:
  explicit AllDifferentNode(std::vector<VariableNode*> variables,
                            VariableNode* r)
      : SoftConstraintNode(variables, r) {
    assert(staticInputs().size() == variables.size());
#ifndef NDEBUG
    for (size_t i = 0; i < variables.size(); ++i) {
      assert(variables[i] = staticInputs()[i]);
    }
#endif
    assert(r == nullptr || violation() == r);
    assert(dynamicInputs().empty());
  }

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;
};
}  // namespace invariantgraph