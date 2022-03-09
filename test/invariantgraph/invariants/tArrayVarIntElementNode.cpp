#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"

class ArrayVarIntElementNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;

  std::shared_ptr<fznparser::SearchVariable> idx;
  std::shared_ptr<fznparser::SearchVariable> y;

  std::unique_ptr<invariantgraph::ArrayVarIntElementNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 3, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 11);
    c = FZN_SEARCH_VARIABLE("c", 1, 9);

    idx = FZN_SEARCH_VARIABLE("idx", 0, 10);
    y = FZN_SEARCH_VARIABLE("y", 0, 10);

    auto constraint =
        makeConstraint("array_var_int_element", FZN_NO_ANNOTATIONS, idx,
                       FZN_VECTOR_CONSTRAINT_ARG(a, b, c), y);

    node = makeNode<invariantgraph::ArrayVarIntElementNode>(constraint);
  }
};

TEST_F(ArrayVarIntElementNodeTest, construction) {
  EXPECT_EQ(node->b()->variable(), idx);
  EXPECT_EQ(node->output()->variable(), y);

  EXPECT_EQ(node->as().size(), 3);
  EXPECT_EQ(node->as()[0]->variable(), a);
  EXPECT_EQ(node->as()[1]->variable(), b);
  EXPECT_EQ(node->as()[2]->variable(), c);
}

TEST_F(ArrayVarIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine);
  node->registerWithEngine(engine, engineVariableMapper);
  engine.close();

  // The index ranges over the variable array (first index is 1).
  EXPECT_EQ(engine.getLowerBound(engineVariable(idx)), 1);
  EXPECT_EQ(engine.getUpperBound(engineVariable(idx)), 3);

  // The output domain should contain all values in all elements of the variable
  // array.
  EXPECT_EQ(engine.getLowerBound(engineVariable(y)), 1);
  EXPECT_EQ(engine.getUpperBound(engineVariable(y)), 11);

  // a, b, c, idx
  EXPECT_EQ(engine.getDecisionVariables().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(engine.getNumVariables(), 5);

  // elementVar
  EXPECT_EQ(engine.getNumInvariants(), 1);
}