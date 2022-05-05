#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/structure.hpp"

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
        _bs(std::move(bs)) {
    assert(staticInputs().size() == _as.size() + _bs.size());
#ifndef NDEBUG
    for (size_t i = 0; i < _as.size(); ++i) {
      assert(_as[i] = staticInputs()[i]);
    }
    for (size_t i = 0; i < _bs.size(); ++i) {
      assert(_bs[i] = staticInputs()[_as.size() + i]);
    }
#endif
    assert(r == nullptr || violation() == r);
    assert(dynamicInputs().empty());
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