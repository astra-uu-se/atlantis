#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantNode.hpp"

namespace invariantgraph {
class ArrayIntMinimumNode : public InvariantNode {
 public:
  ArrayIntMinimumNode(std::vector<VariableNode*> variables,
                      VariableNode* output)
      : InvariantNode({output}, variables) {
#ifndef NDEBUG
    for (auto* const staticInput : staticInputs()) {
      assert(staticInput->isIntVar());
    }
#endif
  }

  ~ArrayIntMinimumNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_int_minimum", 2}};
  }

  static std::unique_ptr<ArrayIntMinimumNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph
