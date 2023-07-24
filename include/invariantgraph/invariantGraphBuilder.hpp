#pragma once

#include <fznparser/model.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "invariantGraph.hpp"
#include "utils/variant.hpp"

namespace invariantgraph {
class InvariantGraphBuilder {
 private:
  fznparser::Model _model;

 public:
  InvariantGraphBuilder(fznparser::Model&&);

  InvariantGraph build();

 private:
  void initVariableNodes();
  void createNodes();

  std::unique_ptr<VariableDefiningNode> makeVariableDefiningNode(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      bool guessDefinedVariable = false);
  std::unique_ptr<ImplicitConstraintNode> makeImplicitConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint);
  std::unique_ptr<SoftConstraintNode> makeSoftConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint);

  template <typename Val>
  VariableNode* nodeForValue(Val val) {
    if (_valueMap.contains(val)) {
      return _valueMap.at(val);
    }

    std::unique_ptr<VariableNode> node;
    if constexpr (std::is_same_v<Int, Val>) {
      node = std::make_unique<VariableNode>(SearchDomain({val}), true);
    } else if constexpr (std::is_same_v<bool, Val>) {
      node = std::make_unique<VariableNode>(
          SearchDomain({1 - static_cast<Int>(val)}), false);
    } else {
      static_assert(!sizeof(Val));
    }

    _valueMap.emplace(val, node.get());
    _variables.push_back(std::move(node));
    return _valueMap.at(val);
  }

  bool isFree(const std::BoolVar& var, const std::vector<VariableNode*>& vars);
  bool isFree(const std::IntVar& var, const std::vector<VariableNode*>& vars);

  bool argumentIsFreeVariable(
      const fznparser::Arg& constraintArg,
      const std::unordered_set<std::string_view>& definedVars);

  bool allArgumentsAreFreeVariables(
      const fznparser::Constraint& constraint,
      const std::unordered_set<std::string_view>& definedVars);
};
}  // namespace invariantgraph