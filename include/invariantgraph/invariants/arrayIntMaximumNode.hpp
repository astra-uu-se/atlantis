#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {
class ArrayIntMaximumNode : public VariableDefiningNode {
 public:
  ArrayIntMaximumNode(std::vector<VariableNode*> variables,
                      VariableNode* output)
      : VariableDefiningNode({output}, variables) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
  }

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_int_maximum", 2}};
  }

  static std::unique_ptr<ArrayIntMaximumNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ~ArrayIntMaximumNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph
