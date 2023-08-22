#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

class ArrayVarIntMinimumTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::IntVar> c;

  std::unique_ptr<invariantgraph::ArrayIntMinimumNode> node;

  void SetUp() override {
    a = intVar(-5, 10, "a");
    b = intVar(0, 5, "b");
    c = intVar(0, 10, "c");

    node = makeNode<invariantgraph::ArrayIntMinimumNode>(_model->addConstraint(
        fznparser::Constraint("array_int_minimum",
                              std::vector<fznparser::Arg>{
                                  fznparser::IntArg{*a}, fznparser::IntArg{*b},
                                  fznparser::IntArg{*c}},
                              std::vector<fznparser::Annotation>{
                                  definesVarAnnotation(c->identifier())})));
  }
};

TEST_F(ArrayVarIntMinimumTest, construction) {
  EXPECT_EQ(*node->staticInputVarNodeIds()[0]->variable(),
            invariantgraph::VarNode::FZNVariable(*a));
  EXPECT_EQ(*node->staticInputVarNodeIds()[1]->variable(),
            invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*c));
  expectMarkedAsInput(node.get(), node->staticInputVarNodeIds());
}

TEST_F(ArrayVarIntMinimumTest, application) {
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

  EXPECT_EQ(engine.lowerBound(engineVariable(*c)), -5);
  EXPECT_EQ(engine.upperBound(engineVariable(*c)), 5);

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // minSparse
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayVarIntMinimumTest, propagation) {
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

      const Int expected = *std::min_element(values.begin(), values.end());
      const Int actual = engine.currentValue(outputId);
      EXPECT_EQ(expected, actual);
    }
  }
}