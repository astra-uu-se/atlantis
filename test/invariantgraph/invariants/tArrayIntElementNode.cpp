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
  EXPECT_EQ(node->output()->variable(), c);

  for (size_t idx = 0; idx < as.size(); idx++) {
    EXPECT_EQ(
        node->as()[idx],
        std::static_pointer_cast<fznparser::ValueLiteral>(as[idx])->value());
  }
}

TEST_F(ArrayIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(0, domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // b
  EXPECT_EQ(engine.getDecisionVariables().size(), 1);

  // b and c
  EXPECT_EQ(engine.getNumVariables(), 2);

  // elementConst
  EXPECT_EQ(engine.getNumInvariants(), 1);
}