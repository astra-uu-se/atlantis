#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/exists.hpp"
#include "invariants/globalCardinalityOpen.hpp"
#include "invariants/linear.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

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
  const std::vector<Int> _cover;
  std::vector<VarId> _intermediate{};
  std::vector<VarId> _violations{};

 public:
  explicit GlobalCardinalityNode(std::vector<VariableNode*> x,
                                 std::vector<Int> cover,
                                 std::vector<VariableNode*> counts,
                                 VariableNode* r)
      : SoftConstraintNode({}, merge(x, counts), r), _cover(cover) {
#ifndef NDEBUG
    size_t i = 0;
    for (auto iter = inputsBegin(); iter != inputsEnd(); ++iter) {
      assert((*iter) == x.at(i));
      i++;
    }
    assert(i == x.size());
    i = 0;
    for (auto iter = countsBegin(); iter != countsEnd(); ++iter) {
      assert((*iter) == counts.at(i));
      i++;
    }
    assert(i == counts.size());
#endif
  }

  explicit GlobalCardinalityNode(std::vector<VariableNode*> x,
                                 std::vector<Int> cover,
                                 std::vector<VariableNode*> counts,
                                 bool shouldHold)
      : SoftConstraintNode(shouldHold ? counts : std::vector<VariableNode*>{},
                           shouldHold ? x : merge(x, counts), shouldHold),
        _cover(cover) {
#ifndef NDEBUG
    size_t i = 0;
    for (auto iter = inputsBegin(); iter != inputsEnd(); ++iter) {
      assert((*iter) == x.at(i));
      i++;
    }
    assert(i == x.size());
    i = 0;
    for (auto iter = countsBegin(); iter != countsEnd(); ++iter) {
      assert((*iter) == counts.at(i));
      i++;
    }
    assert(i == counts.size());
#endif
  }

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

  size_t numInputs() const {
    if (!isReified() && shouldHold()) {
      return staticInputs().size();
    }
    return staticInputs().size() - _cover.size();
  }

  std::vector<VariableNode*>::const_iterator inputsBegin() {
    return staticInputs().begin();
  }

  std::vector<VariableNode*>::const_iterator inputsEnd() {
    if (!isReified() && shouldHold()) {
      return staticInputs().end();
    }
    return staticInputs().end() - _cover.size();
  }

  std::vector<VariableNode*>::const_iterator countsBegin() {
    if (!isReified() && shouldHold()) {
      return definedVariables().begin();
    }
    return staticInputs().end() - _cover.size();
  }

  std::vector<VariableNode*>::const_iterator countsEnd() {
    if (!isReified() && shouldHold()) {
      return definedVariables().end();
    }
    return staticInputs().end();
  }
};
}  // namespace invariantgraph