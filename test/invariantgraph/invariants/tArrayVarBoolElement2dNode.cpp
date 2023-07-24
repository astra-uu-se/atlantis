#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayVarBoolElement2dNode.hpp"

class ArrayVarBoolElement2dNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(x00);
  BOOL_VARIABLE(x01);
  BOOL_VARIABLE(x10);
  BOOL_VARIABLE(x11);

  INT_VARIABLE(idx1, 1, 2);
  INT_VARIABLE(idx2, 1, 2);
  INT_VARIABLE(y, 0, 10);

  fznparser::Constraint constraint{
      "array_var_bool_element2d_nonshifted_flat",
      {"idx1", "idx2",
       fznparser::Constraint::ArrayArgument{"x00", "x01", "x10", "x11"}, "y", 2,
       1, 1},
      {}};

  fznparser::Model model{{},
                         {x00, x01, x10, x11, idx1, idx2, y},
                         {constraint},
                         fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayVarBoolElement2dNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::ArrayVarBoolElement2dNode>(constraint);
  }
};

TEST_F(ArrayVarBoolElement2dNodeTest, construction) {
  EXPECT_EQ(*node->idx1()->variable(),
            invariantgraph::VariableNode::FZNVariable(idx1));
  EXPECT_EQ(*node->idx2()->variable(),
            invariantgraph::VariableNode::FZNVariable(idx2));

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(y));
  expectMarkedAsInput(node.get(), {node->dynamicInputs()});
  expectMarkedAsInput(node.get(), {node->idx1()});
  expectMarkedAsInput(node.get(), {node->idx2()});

  EXPECT_EQ(node->dynamicInputs().size(), 4);
  EXPECT_EQ(node->dynamicInputs().at(0)->variable(),
            invariantgraph::VariableNode::FZNVariable(x00));
  EXPECT_EQ(node->dynamicInputs().at(1)->variable(),
            invariantgraph::VariableNode::FZNVariable(x01));
  EXPECT_EQ(node->dynamicInputs().at(2)->variable(),
            invariantgraph::VariableNode::FZNVariable(x10));
  EXPECT_EQ(node->dynamicInputs().at(3)->variable(),
            invariantgraph::VariableNode::FZNVariable(x11));
}

TEST_F(ArrayVarBoolElement2dNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(
      engine, {x00.name, x01.name, x10.name, x11.name, idx1.name, idx2.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

  // x00, x01, x10, x11, idx1, idx2
  EXPECT_EQ(engine.searchVariables().size(), 6);

  // x00, x01, x10, x11, idx1, idx2, and y
  EXPECT_EQ(engine.numVariables(), 7);

  // element2dVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayVarBoolElement2dNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(
      engine, {x00.name, x01.name, x10.name, x11.name, idx1.name, idx2.name});
  node->createDefinedVariables(engine);
  node->registerWithEngine(engine);

  EXPECT_EQ(node->staticInputs().size(), 2);
  EXPECT_NE(node->staticInputs().front()->varId(), NULL_ID);

  EXPECT_EQ(node->dynamicInputs().size(), 4);
  for (auto* const inputVariable : node->dynamicInputs()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
  }

  EXPECT_NE(node->definedVariables().front()->varId(), NULL_ID);
  const VarId outputId = node->definedVariables().front()->varId();

  std::vector<VarId> inputs;
  inputs.emplace_back(node->idx1()->varId());
  inputs.emplace_back(node->idx2()->varId());
  for (auto* const varNode : node->dynamicInputs()) {
    inputs.emplace_back(varNode->varId());
    engine.updateBounds(varNode->varId(), 0, 3, true);
  }
  engine.close();
  std::vector<Int> values(inputs.size(), 0);

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      for (values.at(2) = engine.lowerBound(inputs.at(2));
           values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
        for (values.at(3) = engine.lowerBound(inputs.at(3));
             values.at(3) <= engine.upperBound(inputs.at(3)); ++values.at(3)) {
          for (values.at(4) = engine.lowerBound(inputs.at(4));
               values.at(4) <= engine.upperBound(inputs.at(4));
               ++values.at(4)) {
            for (values.at(5) = engine.lowerBound(inputs.at(5));
                 values.at(5) <= engine.upperBound(inputs.at(5));
                 ++values.at(5)) {
              engine.beginMove();
              for (size_t i = 0; i < inputs.size(); ++i) {
                engine.setValue(inputs.at(i), values.at(i));
              }
              engine.endMove();

              engine.beginProbe();
              engine.query(outputId);
              engine.endProbe();

              const Int actual = engine.currentValue(outputId);
              const Int row = values.at(0) - 1;  // offset of 1
              const Int col = values.at(1) - 1;  // offset of 1

              const Int index = 2 + (row * 2 + col);

              EXPECT_EQ(actual, values.at(index));
            }
          }
        }
      }
    }
  }
}