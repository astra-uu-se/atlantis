#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/exists.hpp"
#include "invariants/globalCardinalityOpen.hpp"
#include "invariants/linear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

static std::vector<invariantgraph::VariableNode*> merge(
    const std::vector<invariantgraph::VariableNode*>& fst,
    const std::vector<invariantgraph::VariableNode*>& snd) {
  std::vector<invariantgraph::VariableNode*> v(fst);
  v.reserve(fst.size() + snd.size());
  v.insert(v.end(), snd.begin(), snd.end());
  return v;
}

namespace invariantgraph {
class GlobalCardinalityNode : public SoftConstraintNode {
 private:
  const std::vector<VariableNode*> _inputs;
  const std::vector<Int> _cover;
  const std::vector<VariableNode*> _counts;
  std::vector<VarId> _intermediate{};
  std::vector<VarId> _violations{};

 public:
  explicit GlobalCardinalityNode(std::vector<VariableNode*> x,
                                 std::vector<Int> cover,
                                 std::vector<VariableNode*> counts,
                                 VariableNode* r)
      : SoftConstraintNode({}, merge(x, counts), r),
        _inputs(x),
        _cover(cover),
        _counts(counts) {}

  explicit GlobalCardinalityNode(std::vector<VariableNode*> x,
                                 std::vector<Int> cover,
                                 std::vector<VariableNode*> counts,
                                 bool shouldHold)
      : SoftConstraintNode(shouldHold ? counts : std::vector<VariableNode*>{},
                           shouldHold ? x : merge(x, counts), shouldHold),
        _inputs(x),
        _cover(cover),
        _counts(counts) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_global_cardinality", 3}, {"fzn_global_cardinality_reif", 4}};
  }

  static std::unique_ptr<GlobalCardinalityNode> fromModelConstraint(
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