#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/globalCardinalityConst.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/exists.hpp"
#include "invariants/linear.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {
class GlobalCardinalityLowUpNode : public SoftConstraintNode {
 private:
  const std::vector<VariableNode*> _inputs;
  const std::vector<Int> _cover;
  const std::vector<Int> _low;
  const std::vector<Int> _up;
  VarId _intermediate{NULL_ID};

 public:
  explicit GlobalCardinalityLowUpNode(std::vector<VariableNode*> x,
                                      std::vector<Int> cover,
                                      std::vector<Int> low, std::vector<Int> up,
                                      VariableNode* r)
      : SoftConstraintNode({}, x, r),
        _inputs(x),
        _cover(cover),
        _low(low),
        _up(up) {}

  explicit GlobalCardinalityLowUpNode(std::vector<VariableNode*> x,
                                      std::vector<Int> cover,
                                      std::vector<Int> low, std::vector<Int> up,
                                      bool shouldHold)
      : SoftConstraintNode({}, x, shouldHold),
        _inputs(x),
        _cover(cover),
        _low(low),
        _up(up) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_global_cardinality_low_up", 4},
        {"fzn_global_cardinality_low_up_reif", 5}};
  }

  static std::unique_ptr<GlobalCardinalityLowUpNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph