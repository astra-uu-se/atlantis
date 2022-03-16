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
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), y);
  expectMarkedAsInput(node.get(), {node->as()});
  expectMarkedAsInput(node.get(), {node->b()});

  EXPECT_EQ(node->as().size(), 3);
  EXPECT_EQ(node->as()[0]->variable(), a);
  EXPECT_EQ(node->as()[1]->variable(), b);
  EXPECT_EQ(node->as()[2]->variable(), c);
}

TEST_F(ArrayVarIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b, c, idx});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // The index ranges over the variable array (first index is 1).
  EXPECT_EQ(engine.lowerBound(engineVariable(idx)), 1);
  EXPECT_EQ(engine.upperBound(engineVariable(idx)), 3);

  // The output domain should contain all values in all elements of the variable
  // array.
  EXPECT_EQ(engine.lowerBound(engineVariable(y)), 1);
  EXPECT_EQ(engine.upperBound(engineVariable(y)), 11);

  // a, b, c, idx
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(engine.numVariables(), 5);

  // elementVar
  EXPECT_EQ(engine.numInvariants(), 1);
}