#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class BoolClauseNode : public SoftConstraintNode {
 private:
  std::vector<VariableNode*> _as;
  std::vector<VariableNode*> _bs;

 public:
  explicit BoolClauseNode(std::vector<VariableNode*> as,
                          std::vector<VariableNode*> bs)
      : SoftConstraintNode({as}), _as(std::move(as)), _bs(std::move(bs)) {
    for (const auto& b : _bs) {
      b->markAsInputFor(static_cast<VariableDefiningNode*>(this));
    }
  }

  static std::unique_ptr<BoolClauseNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() { return _as; }

  [[nodiscard]] const std::vector<VariableNode*>& bs() { return _bs; }
};
}  // namespace invariantgraph