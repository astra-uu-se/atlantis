#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"

TEST(InvariantGraphBuilderTest, build_simple_graph) {
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

  std::vector<std::shared_ptr<fznparser::Literal>> coeffs{
      std::make_shared<fznparser::ValueLiteral>(1),
      std::make_shared<fznparser::ValueLiteral>(1),
      std::make_shared<fznparser::ValueLiteral>(-1)};

  std::vector<std::shared_ptr<fznparser::Literal>> vars{a, b, c};
  auto sum = std::make_shared<fznparser::ValueLiteral>(0);

  auto constraint = std::make_shared<fznparser::Constraint>(
      "int_lin_eq",
      std::vector<fznparser::ConstraintArgument>{coeffs, vars, sum},
      constraintAnnotations);

  auto model = std::make_unique<fznparser::Model>(
      std::vector<fznparser::Parameter>(),
      std::vector<std::shared_ptr<fznparser::Variable>>{
          std::static_pointer_cast<fznparser::Variable>(a),
          std::static_pointer_cast<fznparser::Variable>(b),
          std::static_pointer_cast<fznparser::Variable>(c)},
      std::vector<std::shared_ptr<fznparser::Constraint>>{constraint});

  invariantgraph::InvariantGraphBuilder builder;
  auto graph = builder.build(model);

  PropagationEngine engine;
  graph->apply(engine);

  EXPECT_EQ(engine.searchVariables().size(), 2);  // a and b
  EXPECT_EQ(engine.numInvariants(), 2);  // totalViolations and int_lin_eq
}

TEST(InvariantGraphBuilderTest, build_simple_graph_2) {
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

  std::vector<std::shared_ptr<fznparser::Literal>> coeffs{
      std::make_shared<fznparser::ValueLiteral>(1),
      std::make_shared<fznparser::ValueLiteral>(1),
      std::make_shared<fznparser::ValueLiteral>(-1)};

  std::vector<std::shared_ptr<fznparser::Literal>> vars{a, b, c};
  auto sum = std::make_shared<fznparser::ValueLiteral>(0);

  auto intLinEq = std::make_shared<fznparser::Constraint>(
      "int_lin_eq",
      std::vector<fznparser::ConstraintArgument>{coeffs, vars, sum},
      constraintAnnotations);

  auto allDiff = std::make_shared<fznparser::Constraint>(
      "alldifferent", std::vector<fznparser::ConstraintArgument>{std::vector<std::shared_ptr<fznparser::Literal>>{a, b, c}},
      fznparser::AnnotationCollection());

  auto model = std::make_unique<fznparser::Model>(
      std::vector<fznparser::Parameter>(),
      std::vector<std::shared_ptr<fznparser::Variable>>{
          std::static_pointer_cast<fznparser::Variable>(a),
          std::static_pointer_cast<fznparser::Variable>(b),
          std::static_pointer_cast<fznparser::Variable>(c)},
      std::vector<std::shared_ptr<fznparser::Constraint>>{intLinEq, allDiff});

  invariantgraph::InvariantGraphBuilder builder;
  auto graph = builder.build(model);

  PropagationEngine engine;
  graph->apply(engine);

  EXPECT_EQ(engine.searchVariables().size(), 2);  // a and b

  // totalViolations and int_lin_eq and alldifferent (constraints are also
  // invariants, they define their violation)
  EXPECT_EQ(engine.numInvariants(), 3);
}

TEST(InvariantGraphBuilderTest, cycles_are_broken) {}