#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/invariantNode.hpp"

namespace invariantgraph {

class Bool2IntNode : public InvariantNode {
 public:
  Bool2IntNode(VariableNode* staticInput, VariableNode* output)
      : InvariantNode({output}, {staticInput}) {}

  ~Bool2IntNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"bool2int", 2}};
  }

  static std::unique_ptr<Bool2IntNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* input() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph