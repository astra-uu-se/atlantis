#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {
class ArrayIntMinimumNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<ArrayIntMinimumNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayIntMinimumNode(std::vector<VariableNode*> variables,
                      VariableNode* output)
      : VariableDefiningNode({output}, variables) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~ArrayIntMinimumNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph
