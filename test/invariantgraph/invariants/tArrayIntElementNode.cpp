#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"

class ArrayIntElementNodeTest : public NodeTestBase {
 public:
  std::vector<std::shared_ptr<fznparser::Literal>> as = {
      std::make_shared<fznparser::ValueLiteral>(1),
      std::make_shared<fznparser::ValueLiteral>(2),
      std::make_shared<fznparser::ValueLiteral>(3)};

  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;

  std::unique_ptr<invariantgraph::ArrayIntElementNode> node;

  void SetUp() override {
    b = FZN_SEARCH_VARIABLE("b", 0, 10);
    c = FZN_SEARCH_VARIABLE("c", 0, 10);

    auto constraint =
        makeConstraint("array_int_element", FZN_NO_ANNOTATIONS, b, as, c);

    node = makeNode<invariantgraph::ArrayIntElementNode>(constraint);
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  EXPECT_EQ(node->b()->variable(), b);
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), c);
  expectMarkedAsInput(node.get(), {node->b()});

  for (size_t idx = 0; idx < as.size(); idx++) {
    EXPECT_EQ(
        node->as()[idx],
        std::static_pointer_cast<fznparser::ValueLiteral>(as[idx])->value());
  }
}

TEST_F(ArrayIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // The index ranges over the as array (first index is 1).
  EXPECT_EQ(engine.getLowerBound(engineVariable(b)), 1);
  EXPECT_EQ(engine.getUpperBound(engineVariable(b)), as.size());

  // The output domain should contain all elements in as.
  EXPECT_EQ(engine.getLowerBound(engineVariable(c)), 1);
  EXPECT_EQ(engine.getUpperBound(engineVariable(c)), 3);

  // b
  EXPECT_EQ(engine.getDecisionVariables().size(), 1);

  // b and c
  EXPECT_EQ(engine.getNumVariables(), 2);

  // elementConst
  EXPECT_EQ(engine.getNumInvariants(), 1);
}