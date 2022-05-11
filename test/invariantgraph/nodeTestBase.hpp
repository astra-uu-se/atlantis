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

template <typename T>
fznparser::Identifier identifier(const T& variable) {
  return std::visit<fznparser::Identifier>(
      [](const auto& var) { return var.name; }, variable);
}

class NodeTestBase : public testing::Test {
 protected:
  fznparser::FZNModel* _model;
  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;
  std::unordered_map<fznparser::Identifier, invariantgraph::VariableNode*>
      _nodeMap;

  void setModel(fznparser::FZNModel* model) { _model = model; }

  std::function<invariantgraph::VariableNode*(invariantgraph::MappableValue&)>
      nodeFactory = [&](const auto& mappable) -> invariantgraph::VariableNode* {
    auto n = std::visit<std::unique_ptr<invariantgraph::VariableNode>>(
        overloaded{
            [&](const fznparser::Identifier& identifier) {
              auto variable =
                  std::get<fznparser::Variable>(*_model->identify(identifier));
              return std::visit<std::unique_ptr<invariantgraph::VariableNode>>(
                  overloaded{
                      [](const fznparser::IntVariable& var) {
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
            },
            [](Int value) {
              return std::make_unique<invariantgraph::VariableNode>(
                  SetDomain({value}));
            },
            [](bool value) {
              return std::make_unique<invariantgraph::VariableNode>(
                  SetDomain({1 - static_cast<int>(value)}));
            }},
        mappable);
    auto ptr = n.get();
    if (n->variable()) {
      _nodeMap.emplace(identifier(*n->variable()), ptr);
    }
    _variables.push_back(std::move(n));
    return ptr;
  };

  void TearDown() override {
    _nodeMap.clear();
    _variables.clear();
  }

  template <typename Node>
  inline std::unique_ptr<Node> makeNode(
      const fznparser::Constraint& constraint) {
    return Node::fromModelConstraint(*_model, constraint, nodeFactory);
  }

  inline void registerVariables(
      PropagationEngine& engine,
      const std::vector<fznparser::Identifier>& freeVariables = {}) {
    for (const auto& modelVariable : freeVariables) {
      auto variable = _nodeMap.at(modelVariable);
      const auto& [lb, ub] = variable->bounds();
      variable->setVarId(engine.makeIntVar(lb, lb, ub));
    }
  }

  template <typename T>
  [[nodiscard]] inline VarId engineVariable(
      const fznparser::SearchVariable<T>& variable) const {
    return _nodeMap.at(variable.name)->varId();
  }

  [[nodiscard]] inline VarId engineVariable(
      invariantgraph::VariableNode* node) const {
    return _variableMap.at(node);
  }
};

inline void expectMarkedAsInput(
    invariantgraph::VariableDefiningNode* definingNode,
    const std::vector<invariantgraph::VariableNode*>& inputs) {
  for (const auto& variableNode : inputs) {
    EXPECT_NE(std::find(variableNode->inputFor().begin(),
                        variableNode->inputFor().end(), definingNode),
              variableNode->inputFor().end());
  }
}
