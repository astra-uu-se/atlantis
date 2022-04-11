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
  std::unordered_map<fznparser::Identifier, VariableNode*> _variableMap;

  // Used for values that are used in place of search variables. To reduce
  // memory, if the same value occurs in multiple times, they will use the
  // same variable node.
  std::unordered_map<std::variant<Int, bool>, VariableNode*> _valueMap;

  std::vector<std::unique_ptr<VariableNode>> _variables;
  std::vector<std::unique_ptr<VariableDefiningNode>> _definingNodes;

 public:
  invariantgraph::InvariantGraph build(const fznparser::FZNModel& model);

 private:
  void createNodes(const fznparser::FZNModel& model);

  std::unique_ptr<VariableDefiningNode> makeVariableDefiningNode(
      const fznparser::FZNModel& model,
      const fznparser::Constraint& constraint);
  std::unique_ptr<ImplicitConstraintNode> makeImplicitConstraint(
      const fznparser::FZNModel& model,
      const fznparser::Constraint& constraint);
  std::unique_ptr<SoftConstraintNode> makeSoftConstraint(
      const fznparser::FZNModel& model,
      const fznparser::Constraint& constraint);

  template <typename Val>
  VariableNode* nodeForValue(Val val) {
    if (auto it = _valueMap.find(val); it != _valueMap.end()) {
      return it->second;
    }

    std::unique_ptr<VariableNode> node;
    if constexpr (std::is_same_v<Int, Val>)
      node = std::make_unique<VariableNode>(SetDomain({val}));
    else if constexpr (std::is_same_v<bool, Val>)
      node = std::make_unique<VariableNode>(
          SetDomain({1 - static_cast<Int>(val)}));
    else
      static_assert(!sizeof(Val));

    _valueMap.emplace(val, node.get());
    _variables.push_back(std::move(node));
    return _valueMap.at(val);
  }

  VariableNode* nodeForParameter(const fznparser::Parameter& parameter);

  VariableNode* nodeForIdentifier(const fznparser::FZNModel& model,
                                  const fznparser::Identifier& identifier);

  VariableNode* nodeFactory(const fznparser::FZNModel& model,
                            const MappableValue& argument);
};
}  // namespace invariantgraph