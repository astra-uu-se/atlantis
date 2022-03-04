#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intLinLeReifNode.hpp"

class IntLinLeReifNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;
  std::shared_ptr<fznparser::SearchVariable> r;

  std::unique_ptr<invariantgraph::IntLinLeReifNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    c = FZN_VALUE(3);
    r = FZN_SEARCH_VARIABLE("r", 0, 1);

    auto as = FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(-1));
    auto bs = FZN_VECTOR_CONSTRAINT_ARG(a, b);

    auto constraint =
        makeConstraint("int_lin_le_reif", FZN_NO_ANNOTATIONS, as, bs, c, r);

    node = makeNode<invariantgraph::IntLinLeReifNode>(constraint);
  }
};

TEST_F(IntLinLeReifNodeTest, construction) { EXPECT_EQ(node->variable(), r); }

TEST_F(IntLinLeReifNodeTest, application) {
  std::map<invariantgraph::VariableNode*, VarId> variableMap;

  PropagationEngine engine;
  engine.open();
  for (const auto& var : _variables)
    var->registerWithEngine(engine, variableMap);
  node->registerWithEngine(engine, variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.getDecisionVariables().size(), 3);

  // a, b and r
  EXPECT_EQ(engine.getNumVariables(), 5);

  // a, b and r
  EXPECT_EQ(variableMap.size(), 3);
}
