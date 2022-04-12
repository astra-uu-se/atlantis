#pragma once

#include <gtest/gtest.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include "core/propagationEngine.hpp"
#include "fznparser/model.hpp"
#include "invariantgraph/structure.hpp"
#include "utils/variant.hpp"

#define INT_VARIABLE(name, lb, ub)              \
  fznparser::IntVariable name {                 \
#name, fznparser::IntRange{lb, ub }, {}, {} \
  }

#define BOOL_VARIABLE(name)                         \
  fznparser::BoolVariable name {                    \
#name, fznparser::BasicDomain < bool>{}, {}, {} \
  }

class NodeTestBase : public testing::Test {
 protected:
  fznparser::FZNModel& _model;
  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;
  std::unordered_map<fznparser::Identifier, invariantgraph::VariableNode*>
      _nodeMap;
  std::unordered_map<invariantgraph::VariableNode*, VarId> _variableMap;

  explicit NodeTestBase(fznparser::FZNModel& model) : _model(model) {}

  std::function<invariantgraph::VariableNode*(invariantgraph::MappableValue&)>
      nodeFactory = [&](const auto& mappable) -> invariantgraph::VariableNode* {
    auto identifier = std::get<fznparser::Identifier>(mappable);
    auto variable = std::get<fznparser::Variable>(*_model.identify(identifier));
    auto n = std::visit<std::unique_ptr<invariantgraph::VariableNode>>(
        overloaded{[](const fznparser::IntVariable& var) {
                     return std::make_unique<invariantgraph::VariableNode>(
                         invariantgraph::VariableNode::FZNVariable(var));
                   },
                   [](const fznparser::BoolVariable& var) {
                     return std::make_unique<invariantgraph::VariableNode>(
                         invariantgraph::VariableNode::FZNVariable(var));
                   },
                   [](const auto&) {
                     assert(false);
                     return nullptr;
                   }},
        variable);

    auto ptr = n.get();
    _nodeMap.emplace(identifier, ptr);
    _variables.push_back(std::move(n));
    return ptr;
  };

  void TearDown() override {
    _nodeMap.clear();
    _variableMap.clear();
    _variables.clear();
  }

  template <typename Node>
  inline std::unique_ptr<Node> makeNode(
      const fznparser::Constraint& constraint) {
    return Node::fromModelConstraint(_model, constraint, nodeFactory);
  }

  inline void registerVariables(
      PropagationEngine& engine,
      const std::vector<fznparser::Identifier>& freeVariables = {}) {
    for (const auto& modelVariable : freeVariables) {
      auto variable = _nodeMap.at(modelVariable);
      const auto& [lb, ub] = variable->bounds();
      auto varId = engine.makeIntVar(lb, lb, ub);
      _variableMap.emplace(variable, varId);
    }
  }

  [[nodiscard]] inline VarId engineVariable(
      const fznparser::IntVariable& variable) const {
    return _variableMap.at(_nodeMap.at(variable.name));
  }
};

inline void expectMarkedAsInput(
    invariantgraph::VariableDefiningNode* definingNode,
    const std::vector<invariantgraph::VariableNode*>& inputs) {
  for (const auto& variableNode : inputs) {
    EXPECT_EQ(std::count(variableNode->inputFor().begin(),
                         variableNode->inputFor().end(), definingNode),
              1);
  }
}
