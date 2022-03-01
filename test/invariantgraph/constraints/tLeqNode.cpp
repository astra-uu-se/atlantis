#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/leqNode.hpp"

class LeqNodeTest : public ::testing::Test {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;
  std::unique_ptr<invariantgraph::LeqNode> node;

  void SetUp() {
    a = std::make_shared<fznparser::SearchVariable>(
        "a", fznparser::AnnotationCollection(),
        std::make_unique<fznparser::IntDomain>(0, 10));
    b = std::make_shared<fznparser::SearchVariable>(
        "b", fznparser::AnnotationCollection(),
        std::make_unique<fznparser::IntDomain>(0, 10));

    c = std::make_shared<fznparser::ValueLiteral>(3);

    auto constraint = std::make_shared<fznparser::Constraint>(
        "int_lin_le",
        std::vector<fznparser::ConstraintArgument>{
            std::vector<std::shared_ptr<fznparser::Literal>>{
                std::static_pointer_cast<fznparser::Literal>(
                    std::make_shared<fznparser::ValueLiteral>(1)),
                std::static_pointer_cast<fznparser::Literal>(
                    std::make_shared<fznparser::ValueLiteral>(2))},
            std::vector<std::shared_ptr<fznparser::Literal>>{
                std::static_pointer_cast<fznparser::Literal>(a),
                std::static_pointer_cast<fznparser::Literal>(b)},
            c},
        fznparser::AnnotationCollection());

    node = invariantgraph::LeqNode::fromModelConstraint(
        constraint, [&](const auto& var) {
          auto n = std::make_unique<invariantgraph::VariableNode>(
              std::dynamic_pointer_cast<fznparser::SearchVariable>(var));

          auto ptr = n.get();
          _variables.push_back(std::move(n));
          return ptr;
        });
  }
};

TEST_F(LeqNodeTest, construction) {
  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->coeffs()[0], 1);
  EXPECT_EQ(node->coeffs()[1], 2);
  EXPECT_EQ(node->bound(), 3);
}

TEST_F(LeqNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(0, domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // a, b, the bound (which we have to represent as a variable, but it has a
  // unit domain so a search wouldn't use it).
  EXPECT_EQ(engine.getDecisionVariables().size(), 3);

  // a, b, the linear sum of a and b, the bound (we have to represent it as an
  // IntVar), the violation of the <= constraint.
  EXPECT_EQ(engine.getNumVariables(), 5);

  // linear and <=
  EXPECT_EQ(engine.getNumInvariants(), 2);
}
