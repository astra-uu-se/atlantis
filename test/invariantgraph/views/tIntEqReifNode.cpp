#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intEqReifNode.hpp"

class IntEqReifNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> r;

  std::unique_ptr<invariantgraph::IntEqReifNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    r = FZN_SEARCH_VARIABLE("r", 0, 1);

    auto constraint = makeConstraint("int_eq_reif", FZN_NO_ANNOTATIONS, a, b, r);

    node = makeNode<invariantgraph::IntEqReifNode>(constraint);
  }
};

TEST_F(IntEqReifNodeTest, construction) {
  EXPECT_EQ(node->variable(), r);
}

TEST_F(IntEqReifNodeTest, application) {
  std::map<invariantgraph::VariableNode*, VarId> variableMap;

  PropagationEngine engine;
  engine.open();
  for (const auto& var : _variables)
    var->registerWithEngine(engine, variableMap);
  node->registerWithEngine(engine, variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.getDecisionVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.getNumVariables(), 3);

  // a, b and r
  EXPECT_EQ(variableMap.size(), 3);
}
