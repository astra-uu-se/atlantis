#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayBoolOrNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _as;

 public:
  static std::unique_ptr<ArrayBoolOrNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolOrNode(std::vector<VariableNode*> as, VariableNode* output)
      : VariableDefiningNode({output}, as), _as(std::move(as)) {
    output->imposeDomain(SetDomain({0, 1}));
  }

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() const noexcept {
    return _as;
  }
};

}  // namespace invariantgraph