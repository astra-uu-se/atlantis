#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/intPowNode.hpp"

class IntPowNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::IntVar> c;

  std::unique_ptr<invariantgraph::IntPowNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(0, 10, "a");
    b = intVar(0, 10, "b");
    c = intVar(0, 10, "c");

    node = makeNode<invariantgraph::IntPowNode>(_model->addConstraint(
        fznparser::Constraint("int_pow",
                              std::vector<fznparser::Arg>{
                                  fznparser::IntArg{*a}, fznparser::IntArg{*b},
                                  fznparser::IntArg{*c}},
                              std::vector<fznparser::Annotation>{
                                  definesVarAnnotation(c->identifier())})));
  }
};

TEST_F(IntPowNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(), invariantgraph::VarNode::FZNVariable(*a));
  EXPECT_EQ(*node->b()->variable(), invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*c));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntPowNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->registerOutputVariables(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerNode(*_invariantGraph, engine);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // intPow
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(IntPowNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  node->registerOutputVariables(engine);
  node->registerNode(*_invariantGraph, engine);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->staticInputVarNodeIds().size(), 2);
  for (auto* const inputVariable : node->staticInputVarNodeIds()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
    inputs.emplace_back(inputVariable->varId());
  }

  EXPECT_NE(node->outputVarNodeIds().front()->varId(), NULL_ID);
  const VarId outputId = node->outputVarNodeIds().front()->varId();
  EXPECT_EQ(inputs.size(), 2);

  std::vector<Int> values(inputs.size());
  engine.close();

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      engine.beginMove();
      for (size_t i = 0; i < inputs.size(); ++i) {
        engine.setValue(inputs.at(i), values.at(i));
      }
      engine.endMove();

      engine.beginProbe();
      engine.query(outputId);
      engine.endProbe();

      if (values.at(0) != 0 || values.at(1) > 0) {
        const Int power = engine.currentValue(outputId);
        EXPECT_EQ(power, std::pow(values.at(0), values.at(1)));
      }
    }
  }
}