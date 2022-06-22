#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/globalCardinalityClosed.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/exists.hpp"
#include "invariants/linear.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class GlobalCardinalityClosedNode : public SoftConstraintNode {
 private:
  const std::vector<VariableNode*> _inputs;
  const std::vector<Int> _cover;
  const std::vector<VariableNode*> _counts;
  std::vector<VarId> _intermediate{};
  std::vector<VarId> _violations{};
  VarId _shouldFailViol{NULL_ID};

 public:
  explicit GlobalCardinalityClosedNode(std::vector<VariableNode*> x,
                                       std::vector<Int> cover,
                                       std::vector<VariableNode*> counts,
                                       VariableNode* r);

  explicit GlobalCardinalityClosedNode(std::vector<VariableNode*> x,
                                       std::vector<Int> cover,
                                       std::vector<VariableNode*> counts,
                                       bool shouldHold);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_global_cardinality_closed", 3},
        {"fzn_global_cardinality_closed_reif", 4}};
  }

  static std::unique_ptr<GlobalCardinalityClosedNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] inline const std::vector<VariableNode*>& inputs() const {
    return _inputs;
  }

  [[nodiscard]] inline const std::vector<VariableNode*>& counts() const {
    return _counts;
  }
};
}  // namespace invariantgraph