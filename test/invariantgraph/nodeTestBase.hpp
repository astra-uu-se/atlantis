#pragma once

#include <gtest/gtest.h>

#include "invariantgraph/structure.hpp"

class NodeTestBase : public testing::Test {
 protected:
  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;

  std::function<invariantgraph::VariableNode*(
      std::shared_ptr<fznparser::Variable>)>
      nodeFactory = [&](const auto& var) {
        auto n = std::make_unique<invariantgraph::VariableNode>(
            std::dynamic_pointer_cast<fznparser::SearchVariable>(var));

        auto ptr = n.get();
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
};

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
