#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"

class Bool2IntNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;

  std::unique_ptr<invariantgraph::Bool2IntNode> node;

  void SetUp() override {
    a = std::make_shared<fznparser::SearchVariable>(
        "a", fznparser::AnnotationCollection(),
        std::make_unique<fznparser::BoolDomain>());
    b = FZN_SEARCH_VARIABLE("b", 0, 1);

    FZN_DEFINES_VAR_ANNOTATION(annotations, b);
    auto constraint = makeConstraint("bool2int", annotations, a, b);

    node = makeNode<invariantgraph::Bool2IntNode>(constraint);
  }
};

TEST_F(Bool2IntNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(), a);
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), b);
}

TEST_F(Bool2IntNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);

  // a and b
  EXPECT_EQ(_variableMap.size(), 2);
}
