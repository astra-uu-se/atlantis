#pragma once

#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/structure.hpp"
#include "invariantgraphTest.hpp"

class NodeTestBase : public testing::Test {
 protected:
  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;
  std::map<std::shared_ptr<fznparser::SearchVariable>,
           invariantgraph::VariableNode*>
      _nodeMap;
  std::map<invariantgraph::VariableNode*, VarId> _variableMap;

  std::function<invariantgraph::VariableNode*(
      std::shared_ptr<fznparser::Variable>)>
      nodeFactory = [&](const auto& var) {
        auto n = std::make_unique<invariantgraph::VariableNode>(
            std::dynamic_pointer_cast<fznparser::SearchVariable>(var));

        auto ptr = n.get();
        _nodeMap.emplace(n->variable(), ptr);
        _variables.push_back(std::move(n));
        return ptr;
      };

  void TearDown() override {
    _nodeMap.clear();
    _variableMap.clear();
    _variables.clear();
  }

  template <typename... Args>
  inline std::shared_ptr<fznparser::Constraint> makeConstraint(
      const std::string& name, fznparser::AnnotationCollection annotations,
      Args... arguments) {
    std::vector<fznparser::ConstraintArgument> args{{arguments...}};
    return std::make_shared<fznparser::Constraint>(name, args, annotations);
  }

  template <typename Node>
  inline std::unique_ptr<Node> makeNode(
      std::shared_ptr<fznparser::Constraint> constraint) {
    return Node::fromModelConstraint(constraint, nodeFactory);
  }

  inline void registerVariables(
      PropagationEngine& engine,
      const std::vector<std::shared_ptr<fznparser::SearchVariable>>&
          freeVariables = {}) {
    for (const auto& modelVariable : freeVariables) {
      auto variable = _nodeMap.at(modelVariable);
      const auto& [lb, ub] = variable->bounds();
      auto varId = engine.makeIntVar(lb, lb, ub);
      _variableMap.emplace(variable, varId);
    }
  }

  [[nodiscard]] inline VarId engineVariable(
      const std::shared_ptr<fznparser::SearchVariable>& variable) const {
    return _variableMap.at(_nodeMap.at(variable));
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
