#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

static std::vector<invariantgraph::VariableNode*> merge(
    const std::vector<invariantgraph::VariableNode*>& as,
    const std::vector<invariantgraph::VariableNode*>& bs) {
  std::vector<invariantgraph::VariableNode*> output(as.size() + bs.size());
  for (size_t i = 0; i < as.size(); ++i) {
    output[i] = as[i];
  }
  for (size_t i = 0; i < bs.size(); ++i) {
    output[as.size() + i] = bs[i];
  }
  return output;
}

namespace invariantgraph {
class BoolClauseNode : public SoftConstraintNode {
 private:
  std::vector<VariableNode*> _as;
  std::vector<VariableNode*> _bs;
  VarId _sumVarId{NULL_ID};

 public:
  explicit BoolClauseNode(std::vector<VariableNode*> as,
                          std::vector<VariableNode*> bs, VariableNode* r)
      : SoftConstraintNode(merge(as, bs), r),
        _as(std::move(as)),
        _bs(std::move(bs)) {}
  explicit BoolClauseNode(std::vector<VariableNode*> as,
                          std::vector<VariableNode*> bs, bool shouldHold)
      : SoftConstraintNode(merge(as, bs), shouldHold),
        _as(std::move(as)),
        _bs(std::move(bs)) {}

  static std::unique_ptr<BoolClauseNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() { return _as; }

  [[nodiscard]] const std::vector<VariableNode*>& bs() { return _bs; }
};
}  // namespace invariantgraph