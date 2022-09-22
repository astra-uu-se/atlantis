#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/invariantNode.hpp"

namespace invariantgraph {

class IntAbsNode : public InvariantNode {
 public:
  IntAbsNode(VariableNode* staticInput, VariableNode* output)
      : InvariantNode({output}, {staticInput}) {}

  ~IntAbsNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_abs", 2}};
  }

  static std::unique_ptr<IntAbsNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* input() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph