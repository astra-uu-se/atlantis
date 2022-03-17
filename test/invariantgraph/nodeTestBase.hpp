#pragma once

#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/structure.hpp"

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
      const auto& [lb, ub] = variable->domain();
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

#define FZN_VALUE(value) std::make_shared<fznparser::ValueLiteral>(value)
#define FZN_SEARCH_VARIABLE(name, lowerBound, upperBound) \
  std::make_shared<fznparser::SearchVariable>(            \
      name, fznparser::AnnotationCollection(),            \
      std::make_unique<fznparser::IntDomain>(lowerBound, upperBound))

#define FZN_VECTOR_CONSTRAINT_ARG(...) \
  std::vector<std::shared_ptr<fznparser::Literal>> { __VA_ARGS__ }

#define FZN_NO_ANNOTATIONS fznparser::AnnotationCollection()
#define FZN_DEFINES_VAR_ANNOTATION(name, variable) \
  fznparser::MutableAnnotationCollection name;     \
  (name).add<fznparser::DefinesVarAnnotation>(variable)
