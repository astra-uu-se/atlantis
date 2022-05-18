#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {
class ArrayIntMaximumNode : public VariableDefiningNode {
 public:
  static std::unique_ptr<ArrayIntMaximumNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayIntMaximumNode(std::vector<VariableNode*> variables,
                      VariableNode* output)
      : VariableDefiningNode({output}, variables) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~ArrayIntMaximumNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph
