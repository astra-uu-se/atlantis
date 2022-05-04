#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class BoolClauseNode : public SoftConstraintNode {
 private:
  std::vector<VariableNode*> _as;
  std::vector<VariableNode*> _bs;
  VarId _sumVarId{NULL_ID};

 public:
  explicit BoolClauseNode(std::vector<VariableNode*> as,
                          std::vector<VariableNode*> bs)
      : SoftConstraintNode({as}), _as(std::move(as)), _bs(std::move(bs)) {
    for (const auto& b : _bs) {
      markAsInput(b);
    }
  }

  static std::unique_ptr<BoolClauseNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() { return _as; }

  [[nodiscard]] const std::vector<VariableNode*>& bs() { return _bs; }
};
}  // namespace invariantgraph