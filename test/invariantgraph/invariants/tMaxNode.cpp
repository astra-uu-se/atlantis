#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/maxNode.hpp"

TEST(MaxNodeTest, int_max_is_parsed_correctly) {
  auto a = std::make_shared<fznparser::SearchVariable>(
      "a", fznparser::AnnotationCollection(),
      std::make_unique<fznparser::IntDomain>(0, 10));
  auto b = std::make_shared<fznparser::SearchVariable>(
      "b", fznparser::AnnotationCollection(),
      std::make_unique<fznparser::IntDomain>(0, 10));
  auto c = std::make_shared<fznparser::SearchVariable>(
      "c", fznparser::AnnotationCollection(),
      std::make_unique<fznparser::IntDomain>(0, 10));

  fznparser::MutableAnnotationCollection constraintAnnotations;
  constraintAnnotations.add<fznparser::DefinesVarAnnotation>(c);
  auto constraint = std::make_shared<fznparser::Constraint>(
      "int_max", std::vector<fznparser::ConstraintArgument>{a, b, c},
      constraintAnnotations);

  // required, else the node will be garbage collected.
  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _nodes;
  auto node = invariantgraph::MaxNode::fromModelConstraint(
      constraint, [&](const auto& var) {
        _nodes.emplace_back(std::make_unique<invariantgraph::VariableNode>(
            std::dynamic_pointer_cast<fznparser::SearchVariable>(var)));
        return _nodes.back().get();
      });

  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->output()->variable(), c);
}

TEST(MaxNodeTest, array_int_maximum_is_parsed_correctly) {
  auto a = std::make_shared<fznparser::SearchVariable>(
      "a", fznparser::AnnotationCollection(),
      std::make_unique<fznparser::IntDomain>(0, 10));
  auto b = std::make_shared<fznparser::SearchVariable>(
      "b", fznparser::AnnotationCollection(),
      std::make_unique<fznparser::IntDomain>(0, 10));
  auto c = std::make_shared<fznparser::SearchVariable>(
      "c", fznparser::AnnotationCollection(),
      std::make_unique<fznparser::IntDomain>(0, 10));

  fznparser::MutableAnnotationCollection constraintAnnotations;
  constraintAnnotations.add<fznparser::DefinesVarAnnotation>(c);
  auto constraint = std::make_shared<fznparser::Constraint>(
      "array_int_maximum",
      std::vector<fznparser::ConstraintArgument>{
          c, std::vector<std::shared_ptr<fznparser::Literal>>{a, b}},
      constraintAnnotations);

  // required, else the node will be garbage collected.
  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _nodes;
  auto node = invariantgraph::MaxNode::fromModelConstraint(
      constraint, [&](const auto& var) {
        _nodes.emplace_back(std::make_unique<invariantgraph::VariableNode>(
            std::dynamic_pointer_cast<fznparser::SearchVariable>(var)));
        return _nodes.back().get();
      });

  EXPECT_EQ(node->variables().size(), 2);
  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->output()->variable(), c);
}