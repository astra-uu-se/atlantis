#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElement2dNode.hpp"

class ArrayVarIntElement2dNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> x00;
  std::unique_ptr<fznparser::IntVar> x01;
  std::unique_ptr<fznparser::IntVar> x10;
  std::unique_ptr<fznparser::IntVar> x11;

  std::unique_ptr<fznparser::IntVar> idx1;
  std::unique_ptr<fznparser::IntVar> idx2;
  std::unique_ptr<fznparser::IntVar> y;

  std::unique_ptr<invariantgraph::ArrayVarIntElement2dNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    x00 = intVar(3, 10, "x00");
    x01 = intVar(2, 11, "x01");
    x10 = intVar(1, 9, "x10");
    x11 = intVar(3, 5, "x11");
    idx1 = intVar(1, 2, "idx1");
    idx2 = intVar(1, 2, "idx2");
    y = intVar(0, 10, "y");

    fznparser::IntVarArray argMatrix("");
    argMatrix.append(*x00);
    argMatrix.append(*x01);
    argMatrix.append(*x10);
    argMatrix.append(*x11);

    node = makeNode<invariantgraph::ArrayVarIntElement2dNode>(
        _model->addConstraint(fznparser::Constraint(
            "array_var_int_element2d_nonshifted_flat",
            std::vector<fznparser::Arg>{
                fznparser::IntArg{*idx1}, fznparser::IntArg{*idx2}, argMatrix,
                fznparser::IntArg{*y}, fznparser::IntArg{2},
                fznparser::IntArg{1}, fznparser::IntArg{1}})));
  }
};

TEST_F(ArrayVarIntElement2dNodeTest, construction) {
  EXPECT_EQ(*node->idx1()->variable(),
            invariantgraph::VarNode::FZNVariable(*idx1));
  EXPECT_EQ(*node->idx2()->variable(),
            invariantgraph::VarNode::FZNVariable(*idx2));

  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*y));

  expectMarkedAsInput(node.get(), {node->dynamicInputVarNodeIds()});
  expectMarkedAsInput(node.get(), {node->idx1()});
  expectMarkedAsInput(node.get(), {node->idx2()});

  EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 4);
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(0)->variable(),
            invariantgraph::VarNode::FZNVariable(*x00));
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(1)->variable(),
            invariantgraph::VarNode::FZNVariable(*x01));
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(2)->variable(),
            invariantgraph::VarNode::FZNVariable(*x10));
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(3)->variable(),
            invariantgraph::VarNode::FZNVariable(*x11));
}

TEST_F(ArrayVarIntElement2dNodeTest, application) {
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

  // x00, x01, x10, x11, idx1, idx2
  EXPECT_EQ(engine.searchVariables().size(), 6);

  // x00, x01, x10, x11, idx1, idx2, and y
  EXPECT_EQ(engine.numVariables(), 7);

  // element2dVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayVarIntElement2dNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  node->registerOutputVariables(engine);
  node->registerNode(*_invariantGraph, engine);

  EXPECT_EQ(node->staticInputVarNodeIds().size(), 2);
  for (auto* const staticInput : node->staticInputVarNodeIds()) {
    EXPECT_NE(staticInput->varId(), NULL_ID);
  }

  EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 4);
  for (auto* const inputVariable : node->dynamicInputVarNodeIds()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
  }

  EXPECT_NE(node->outputVarNodeIds().front()->varId(), NULL_ID);
  const VarId outputId = node->outputVarNodeIds().front()->varId();

  std::vector<VarId> inputs;
  inputs.emplace_back(node->idx1()->varId());
  inputs.emplace_back(node->idx2()->varId());
  for (auto* const varNode : node->dynamicInputVarNodeIds()) {
    inputs.emplace_back(varNode->varId());
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